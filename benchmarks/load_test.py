import aiohttp
import asyncio
import numpy as np
import time
import argparse
import json
import os


async def main():
    t = Tester()
    parser = argparse.ArgumentParser()

    parser.add_argument("url", type=str, help="The domain name to load test, do not include any path.")
    #add later
    #parser.add_argument("model", type=str, help="The URL to load test")
    #parser.add_argument("version", type=str, help="The URL to load test")
    #end add later
    parser.add_argument("numClients", type=int, help="The number of concurrent clients sending requests.")
    parser.add_argument("numTasks", type=int, help="The number of total requests sent to the predict endpoints.")
    parser.add_argument("outputFileDestination", type=str, help="The output json file for the results.")

    args = parser.parse_args()

    base_url = args.url.rstrip("/")
    url = base_url + "/predict"
    num_clients = args.numClients
    num_tasks = args.numTasks
    output_file = args.outputFileDestination

    registration_payload = {
        "name": "test model",
        "version": "v1"
    }

    payload = {
        "model": "test model",
        "version": "v1",
        "input": "test"
    }

    try:
        await register_model(base_url, registration_payload)
        await t.run(url, payload, num_clients, num_tasks)
        server_metrics = await fetch_metrics(base_url)
    except ValueError as e:
        print(e)
        return
    except RuntimeError as e:
        print(e)
        return

    num_successes = t.successes
    num_failures = t.failures
    avg_latency = t.getAverageLatency()
    p50_latency = t.computeP50Latency()
    p95_latency = t.computeP95Latency()
    req_per_sec = t.computeReqPerSec()

    new_entry = {
        "number_of_clients": num_clients,
        "number_of_tasks": num_tasks,
        "number_of_successes": num_successes,
        "number_of_failures": num_failures,
        "average_latency_ms": avg_latency,
        "p50_latency_ms": p50_latency,
        "p95_latency_ms": p95_latency,
        "requests_per_second": req_per_sec,
        "server_metrics": server_metrics
    }

    data = []

    if os.path.exists(output_file):
        try:
            with open(output_file, 'r', encoding='utf-8') as file:
                data = json.load(file)
                if not isinstance(data, list):
                    data = [data]
        except json.JSONDecodeError:
            print("Warning: File was corrupted or empty. Creating a new file.")
            data = []

    data.append(new_entry)

    with open(output_file, 'w', encoding='utf-8') as file:
        json.dump(data, file, indent=4)





class Tester:
    def __init__(self) -> None:
        self.successes = 0
        self.failures = 0
        self._total_latency_ms = 0.0
        self._latencies = []
        self._num_tasks = 0
        self._elapsed_seconds = 0
    
    def reset(self) -> None:
        self.successes = 0
        self.failures = 0
        self._total_latency_ms = 0.0
        self._latencies = []
        self._num_tasks = 0
        self._elapsed_seconds = 0

    def getAverageLatency(self) -> float:
        if(len(self._latencies) == 0):
            return 0.0
        return self._total_latency_ms / len(self._latencies)

    def computeP50Latency(self) -> float:
        if(len(self._latencies) == 0):
            return 0.0
        return float(np.quantile(self._latencies, 0.5))

    def computeP95Latency(self) -> float:
        if(len(self._latencies) == 0):
            return 0.0
        return float(np.quantile(self._latencies, 0.95))
    
    def computeReqPerSec(self) -> float:
        if(self._elapsed_seconds == 0):
            return 0.0
        return self._num_tasks / self._elapsed_seconds

    async def run(self, url: str, payload: dict, numClients: int, numTasks: int) -> None:
        if(numClients < 1):
            raise ValueError("numClients must be greater than 0")
        if(numTasks < 1):
            raise ValueError("numTasks must be greater than 0")
        self._num_tasks = numTasks
        queue = asyncio.Queue()
        for i in range(numTasks):
            await queue.put(i)

        connector = aiohttp.TCPConnector(limit=numClients)

        timeout = aiohttp.ClientTimeout(total=10)
        
        async with aiohttp.ClientSession(connector=connector, timeout=timeout) as session:
            workers = []
            for _ in range(numClients):
                task = asyncio.create_task(self._worker(queue, session, url, payload))
                workers.append(task)

            start_time = time.perf_counter()
            await queue.join()
            self._elapsed_seconds = (time.perf_counter() - start_time)

            for task in workers:
                task.cancel()

            await asyncio.gather(*workers, return_exceptions=True)
        

    async def _worker(self, queue: asyncio.Queue, session: aiohttp.ClientSession, url: str, payload: dict) -> None:
        while True:
            await queue.get()
            try:
               start_time = time.perf_counter()
               async with session.post(url, json=payload) as response:
                    
                    if(response.status != 200):
                        raise Exception

                    await response.json()

                    latency_ms = (time.perf_counter() - start_time) * 1000
                    self._latencies.append(latency_ms)
                    self._total_latency_ms += latency_ms
                    
                    self.successes += 1
            except asyncio.TimeoutError:
                self.failures += 1
                print("Request timed out")
            except Exception:
                self.failures += 1
            finally:
                queue.task_done()


async def register_model(base_url: str, payload: dict) -> None:
    timeout = aiohttp.ClientTimeout(total=10)
    async with aiohttp.ClientSession(timeout=timeout) as session:
        async with session.post(base_url + "/models/register", json=payload) as response:
            if response.status == 200:
                return

            body = await response.text()
            if response.status == 400 and body.strip() == "Model already added":
                return

            raise RuntimeError(
                f"Model registration failed with HTTP {response.status}: {body.strip()}"
            )


async def fetch_metrics(base_url: str) -> dict:
    timeout = aiohttp.ClientTimeout(total=10)
    async with aiohttp.ClientSession(timeout=timeout) as session:
        async with session.get(base_url + "/metrics") as response:
            if response.status != 200:
                body = await response.text()
                raise RuntimeError(
                    f"Metrics request failed with HTTP {response.status}: {body.strip()}"
                )

            metrics = await response.json()

    required_sections = {
        "requests",
        "predictions",
        "models",
        "latency_ms",
        "batching",
        "errors",
    }
    if not isinstance(metrics, dict) or not required_sections.issubset(metrics):
        raise RuntimeError("/metrics returned an unexpected JSON structure")

    return metrics


if __name__ == "__main__":
    asyncio.run(main())
