End points:

/predict
Takes json request in form
{
  "model": "classifier",
  "version": "1",
  "inputs": [
    {
      "name": "input",
      "shape": [1, 4],
      "data": [1.2, 3.4, 5.6, 7.8]
    }
  ]
}