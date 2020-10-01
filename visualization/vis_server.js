const fs = require('fs')
const express = require('express')
const app = express()
const port = 3000

let latestChange = latestSent=Date.now();
function readData() {
  return JSON.parse( fs.readFileSync(__dirname + '/data.json'))
}

fs.watch(__dirname + '/data.json',  { encoding: 'buffer' }, (eventType, filename) => {
  latestChange = Date.now()
});
app.use(express.static('public'))
app.get('/', (req, res) => {
  res.sendFile(__dirname + '/index.html');
})
app.get('/data', (req, res) => {
    data = readData()
    ans = {"changed": (latestSent < latestChange), "data":data};
    res.json(ans)
    latestSent = Date.now()
  })

app.listen(port, () => {
    console.log(`Visualization Server Started At http://localhost:${port}`)
})