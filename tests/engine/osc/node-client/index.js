const OSC = require('osc-js')

const senderPort = 9000 // To Element: OSC Receiver node
const receiverPort = 9001 // From Element: OSC Sender node

const osc = new OSC({
  plugin: new OSC.DatagramPlugin({
    type: 'udp4',
    send: {
      port: senderPort
    },
    open: {
      port: receiverPort
    }
  })
})

function startSender() {

  console.log(`Sending to UDP port ${senderPort}`)

  let i = 0
  let direction = 1

  setInterval(function() {
    i += direction

    const message = new OSC.Message('/midi',  1, 1, 1, i / 100)
    osc.send(message)

    if (i===0 || i===99) direction *= -1
  }, 30)
}

function startReceiver() {

  console.log(`Receiving from UDP port ${receiverPort}`)

  osc.on('*', message => {
    console.log(message.address, message.args)
  })

  osc.on('open', () => {

    console.log('open')

    const message = new OSC.Message('/handshake', 'hi')
    osc.send(message)
  })

  osc.open()
}

startSender()
startReceiver()
