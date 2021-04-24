const OSC = require('osc-js')

const senderPort = 9001 // To Element: OSC Receiver node
const receiverPort = 9002 // From Element: OSC Sender node

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

  let channel = 0
  let note = 0
  let velocity = 0
  let command

  let i = 0
  let direction = 1

  setInterval(function() {

    channel = (i % 15) + 1 // Must be 1~16
    note = i % 126
    velocity = i  / 100
    command = direction === 1 ? 'noteOn' : 'noteOff'

    i += direction

    const message = new OSC.Message(`/midi/${command}`,  channel, note, velocity) // new OSC.AtomicFloat32
    osc.send(message)

    if (i===0 || i===99) direction *= -1
  }, 30)
}

function startReceiver() {

  console.log(`Receiving from UDP port ${receiverPort}`)

  osc.on('*', (message, rinfo) => {
    const from = rinfo ? `${rinfo.address}:${rinfo.port} ` : ''
    console.log(from+message.address, message.args)
  })

  osc.on('open', () => {

    console.log('Receiver port open')

    const message = new OSC.Message('/handshake', 'hi')
    osc.send(message)
  })

  osc.open()
}

startSender()
startReceiver()
