--- Mackie Control Universal.
--
-- This is a MIDI filter which forces a specified channel on all messages. Set
-- the channel parameter to '0' to bypass the filter.
--
-- @script      channelize
-- @type        ControlSurface
-- @license     GPL v3
-- @author      Michael Fisher

-- A Channel is used to define which Fader, Meter or VPot should be modified.
-- type Channel byte.

local Channel1 = 0
local Channel2 = 1
local Channel3 = 2
local Channel4 = 3
local Channel5 = 4
local Channel6 = 5
local Channel7 = 6
local Channel8 = 7
-- Master is only a fader and will do nothing if used to set a VPot or a Meter.
local Master   = 8

local LenChannels = 9
local FaderMax = 16382
local FaderMin = 0


-- var (
-- 	Channels = map[int]Channel{
-- 		1: 0x10,
-- 		2: 0x11,
-- 		3: 0x12,
-- 		4: 0x13,
-- 		5: 0x14,
-- 		6: 0x15,
-- 		7: 0x16,
-- 		8: 0x17,
-- 		9: 0x18,
-- 	}
-- )




-- // Char is an ASCII character between 0x21 (33) and 0x60 (96).
-- // Note: if the character is above 0x40 (64) subtract 0x40 from it, otherwise the wrong character will be displayed.
-- type Char byte

-- const (
-- 	SymbolAt Char = iota
-- 	CharA
-- 	CharB
-- 	CharC
-- 	CharD
-- 	CharE
-- 	CharF
-- 	CharG
-- 	CharH
-- 	CharI
-- 	CharJ
-- 	CharK
-- 	CharL
-- 	CharM
-- 	CharN
-- 	CharO
-- 	CharP
-- 	CharQ
-- 	CharR
-- 	CharS
-- 	CharT
-- 	CharU
-- 	CharV
-- 	CharW
-- 	CharX
-- 	CharY
-- 	CharZ
-- 	SymbolOpenBracket
-- 	SymbolBackSlash
-- 	SymbolCloseBracket
-- 	SymbolAccent
-- 	SymbolUnderscore
-- 	SymbolSpace
-- 	SymbolExclamation
-- 	SymbolDoubleQuote
-- 	SymbolNumberSign
-- 	SymbolDollar
-- 	SymbolPercentSign
-- 	SymbolAmpersand
-- 	SymbolSingleQuote
-- 	SymbolOpenParen
-- 	SymbolCloseParen
-- 	SymbolAsterisk
-- 	SymbolPlus
-- 	SymbolComma
-- 	SymbolHyphen
-- 	SymbolPeriod
-- 	SymbolForwardSlash
-- 	Char0
-- 	Char1
-- 	Char2
-- 	Char3
-- 	Char4
-- 	Char5
-- 	Char6
-- 	Char7
-- 	Char8
-- 	Char9
-- 	SymbolColon
-- 	SymbolSemiColon
-- 	SymbolLessThan
-- 	SymbolEqual
-- 	SymbolGreaterThan
-- 	SymbolQuestion
-- 	// Add DigitDot to the character you want to display to show the dot found on each 7-segment display.
-- 	DigitDot
-- )

-- var (
-- 	Letters = map[rune]Char{
-- 		'@':  0x00,
-- 		'A':  0x01,
-- 		'B':  0x02,
-- 		'C':  0x03,
-- 		'D':  0x04,
-- 		'E':  0x05,
-- 		'F':  0x06,
-- 		'G':  0x07,
-- 		'H':  0x08,
-- 		'I':  0x09,
-- 		'J':  0x0A,
-- 		'K':  0x0B,
-- 		'L':  0x0C,
-- 		'M':  0x0D,
-- 		'N':  0x0E,
-- 		'O':  0x0F,
-- 		'P':  0x10,
-- 		'Q':  0x11,
-- 		'R':  0x12,
-- 		'S':  0x13,
-- 		'T':  0x14,
-- 		'U':  0x15,
-- 		'V':  0x16,
-- 		'W':  0x17,
-- 		'X':  0x18,
-- 		'Y':  0x19,
-- 		'Z':  0x1A,
-- 		'[':  0x1B,
-- 		'\\': 0x1C,
-- 		']':  0x1D,
-- 		'^':  0x1E,
-- 		'_':  0x1F,
-- 		' ':  0x20,
-- 		'!':  0x21,
-- 		'"':  0x22,
-- 		'#':  0x23,
-- 		'$':  0x24,
-- 		'%':  0x25,
-- 		'&':  0x26,
-- 		'\'': 0x27,
-- 		'(':  0x28,
-- 		')':  0x29,
-- 		'*':  0x2A,
-- 		'+':  0x2B,
-- 		',':  0x2C,
-- 		'-':  0x2D,
-- 		'.':  0x2E,
-- 		'/':  0x2F,
-- 		'0':  0x30,
-- 		'1':  0x31,
-- 		'2':  0x32,
-- 		'3':  0x33,
-- 		'4':  0x34,
-- 		'5':  0x35,
-- 		'6':  0x36,
-- 		'7':  0x37,
-- 		'8':  0x38,
-- 		'9':  0x39,
-- 		':':  0x3A,
-- 		';':  0x3B,
-- 		'<':  0x3C,
-- 		'=':  0x3D,
-- 		'>':  0x3E,
-- 		'?':  0x3F,
-- 	}
-- )

-- package gomcu

-- type Line byte

-- const (
-- 	Line1    Line = 0
-- 	Line2    Line = 56
-- 	LenLines      = 112
-- )


-- package gomcu

-- // MeterLevel is the height (in decibels) that the meter should be set to.
-- type MeterLevel byte

-- const (
-- 	LessThan60 MeterLevel = iota
-- 	MoreThan60
-- 	MoreThan50
-- 	MoreThan40
-- 	MoreThan30
-- 	MoreThan20
-- 	MoreThan14
-- 	MoreThan10
-- 	MoreThan8
-- 	MoreThan6
-- 	MoreThan4
-- 	MoreThan2
-- 	MoreThan0
-- 	_
-- 	// Clipping enables the clipping warning LED on the meter.
-- 	Clipping
-- 	// ClipOff disables the clipping warning LED on the meter.
-- 	ClipOff

-- 	LenMeterLevel = 15
-- )


-- package gomcu

-- type State byte

-- const (
-- 	StateOff      State = 0x00
-- 	StateBlinking State = 0x01
-- 	StateOn       State = 0x7F
-- )


-- package gomcu

-- // A Switch is a button and/or LED that can be pressed and or lit.
-- type Switch byte

-- const (
-- 	Rec1 Switch = iota
-- 	Rec2
-- 	Rec3
-- 	Rec4
-- 	Rec5
-- 	Rec6
-- 	Rec7
-- 	Rec8
-- 	Solo1
-- 	Solo2
-- 	Solo3
-- 	Solo4
-- 	Solo5
-- 	Solo6
-- 	Solo7
-- 	Solo8
-- 	Mute1
-- 	Mute2
-- 	Mute3
-- 	Mute4
-- 	Mute5
-- 	Mute6
-- 	Mute7
-- 	Mute8
-- 	Select1
-- 	Select2
-- 	Select3
-- 	Select4
-- 	Select5
-- 	Select6
-- 	Select7
-- 	Select8
-- 	V1
-- 	V2
-- 	V3
-- 	V4
-- 	V5
-- 	V6
-- 	V7
-- 	V8
-- 	AssignTrack
-- 	AssignSend
-- 	AssignPan
-- 	AssignPlugin
-- 	AssignEQ
-- 	AssignInstrument
-- 	BankL
-- 	BankR
-- 	ChannelL
-- 	ChannelR
-- 	Flip
-- 	GlobalView
-- 	NameValue
-- 	SMPTEBeats
-- 	F1
-- 	F2
-- 	F3
-- 	F4
-- 	F5
-- 	F6
-- 	F7
-- 	F8
-- 	MIDITracks
-- 	Inputs
-- 	AudioTracks
-- 	AudioInstrument
-- 	Aux
-- 	Busses
-- 	Outputs
-- 	User
-- 	Shift
-- 	Option
-- 	Control
-- 	CMDAlt
-- 	Read
-- 	Write
-- 	Trim
-- 	Touch
-- 	Latch
-- 	Group
-- 	Save
-- 	Undo
-- 	Cancel
-- 	Enter
-- 	Marker
-- 	Nudge
-- 	Cycle
-- 	Drop
-- 	Replace
-- 	Click
-- 	Solo
-- 	Rewind
-- 	FastFwd
-- 	Stop
-- 	Play
-- 	Record
-- 	Up
-- 	Down
-- 	Left
-- 	Right
-- 	Zoom
-- 	Scrub
-- 	UserA
-- 	UserB
-- 	Fader1
-- 	Fader2
-- 	Fader3
-- 	Fader4
-- 	Fader5
-- 	Fader6
-- 	Fader7
-- 	Fader8
-- 	FaderMaster
-- 	STMPELED
-- 	BeatsLED
-- 	RudeSoloLED
-- 	RelayClickLED

-- 	LenIDs = 120
-- )

-- var (
-- 	IDs = map[string]Switch{
-- 		"Rec1":             0x00,
-- 		"Rec2":             0x01,
-- 		"Rec3":             0x02,
-- 		"Rec4":             0x03,
-- 		"Rec5":             0x04,
-- 		"Rec6":             0x05,
-- 		"Rec7":             0x06,
-- 		"Rec8":             0x07,
-- 		"Solo1":            0x08,
-- 		"Solo2":            0x09,
-- 		"Solo3":            0x0A,
-- 		"Solo4":            0x0B,
-- 		"Solo5":            0x0C,
-- 		"Solo6":            0x0D,
-- 		"Solo7":            0x0E,
-- 		"Solo8":            0x0F,
-- 		"Mute1":            0x10,
-- 		"Mute2":            0x11,
-- 		"Mute3":            0x12,
-- 		"Mute4":            0x13,
-- 		"Mute5":            0x14,
-- 		"Mute6":            0x15,
-- 		"Mute7":            0x16,
-- 		"Mute8":            0x17,
-- 		"Select1":          0x18,
-- 		"Select2":          0x19,
-- 		"Select3":          0x1A,
-- 		"Select4":          0x1B,
-- 		"Select5":          0x1C,
-- 		"Select6":          0x1D,
-- 		"Select7":          0x1E,
-- 		"Select8":          0x1F,
-- 		"V1":               0x20,
-- 		"V2":               0x21,
-- 		"V3":               0x22,
-- 		"V4":               0x23,
-- 		"V5":               0x24,
-- 		"V6":               0x25,
-- 		"V7":               0x26,
-- 		"V8":               0x27,
-- 		"AssignTrack":      0x28,
-- 		"AssignSend":       0x29,
-- 		"AssignPan":        0x2A,
-- 		"AssignPlugin":     0x2B,
-- 		"AssignEQ":         0x2C,
-- 		"AssignInstrument": 0x2D,
-- 		"BankL":            0x2E,
-- 		"BankR":            0x2F,
-- 		"ChannelL":         0x30,
-- 		"ChannelR":         0x31,
-- 		"Flip":             0x32,
-- 		"GlobalView":       0x33,
-- 		"Name/Value":       0x34,
-- 		"SMPTE/Beats":      0x35,
-- 		"F1":               0x36,
-- 		"F2":               0x37,
-- 		"F3":               0x38,
-- 		"F4":               0x39,
-- 		"F5":               0x3A,
-- 		"F6":               0x3B,
-- 		"F7":               0x3C,
-- 		"F8":               0x3D,
-- 		"MIDITracks":       0x3E,
-- 		"Inputs":           0x3F,
-- 		"AudioTracks":      0x40,
-- 		"AudioInstrument":  0x41,
-- 		"Aux":              0x42,
-- 		"Busses":           0x43,
-- 		"Outputs":          0x44,
-- 		"User":             0x45,
-- 		"Shift":            0x46,
-- 		"Option":           0x47,
-- 		"Control":          0x48,
-- 		"CMD":              0x49,
-- 		"Read":             0x4A,
-- 		"Write":            0x4B,
-- 		"Trim":             0x4C,
-- 		"Touch":            0x4D,
-- 		"Latch":            0x4E,
-- 		"Group":            0x4F,
-- 		"Save":             0x50,
-- 		"Undo":             0x51,
-- 		"Cancel":           0x52,
-- 		"Enter":            0x53,
-- 		"Marker":           0x54,
-- 		"Nudge":            0x55,
-- 		"Cycle":            0x56,
-- 		"Drop":             0x57,
-- 		"Replace":          0x58,
-- 		"Click":            0x59,
-- 		"Solo":             0x5A,
-- 		"Rewind":           0x5B,
-- 		"FastFwd":          0x5C,
-- 		"Stop":             0x5D,
-- 		"Play":             0x5E,
-- 		"Record":           0x5F,
-- 		"Up":               0x60,
-- 		"Down":             0x61,
-- 		"Left":             0x62,
-- 		"Right":            0x63,
-- 		"Zoom":             0x64,
-- 		"Scrub":            0x65,
-- 		"UserA":            0x66,
-- 		"UserB":            0x67,
-- 		"Fader1":           0x68,
-- 		"Fader2":           0x69,
-- 		"Fader3":           0x6A,
-- 		"Fader4":           0x6B,
-- 		"Fader5":           0x6C,
-- 		"Fader6":           0x6D,
-- 		"Fader7":           0x6E,
-- 		"Fader8":           0x6F,
-- 		"FaderMaster":      0x70,
-- 		"STMPELED":         0x71,
-- 		"BeatsLED":         0x72,
-- 		"RudeSoloLED":      0x73,
-- 		"RelayClickLED":    0x74,
-- 	}
-- )



-- package gomcu

-- type VPotLED byte

-- const (
-- 	VPot0 VPotLED = iota
-- 	VPot1
-- 	VPot2
-- 	VPot3
-- 	VPot4
-- 	VPot5
-- 	VPot6
-- 	VPot7
-- 	VPot8
-- 	VPot9
-- 	VPot10
-- 	VPot11

-- 	LenVPots = 12

-- 	// VPotDot is the bottom LED separate from the other 11.
-- 	VPotDot VPotLED = 0x40
-- )


-- VPot Mode is the display mode that should be used when setting the amount of VPot LEDs to display.
local VPotMode0 = 0x00
local VPotMode1 = 0x10
local VPotMode2 = 0x20
local VPotMode3 = 0x30

local function parameters()
    return {
        {
            name        = "Power",
            label       = "power",
            min         = 0,
            max         = 1,
            default     = 0
        }
    }
end



local bytes         = require ('el.bytes')
local script        = require ('el.script')
local MidiBuffer    = require ('el.MidiBuffer')

local output        = MidiBuffer.new (128)
local sysex         = bytes.new (4)
bytes.set (sysex, 1, 0xF0) -- sysex start
bytes.set (sysex, 2, 0x7D) -- Mfg. ID for educational/development purposes
bytes.set (sysex, 3, 0x00) -- body/value
bytes.set (sysex, 4, 0xF7) -- sysex end

local laststate = bytes.get (sysex, 3)

local function layout()
    return {
        audio = { 0, 0 },
        midi  = { 1, 1 }
    }
end

local function parameters()
    return {
        {
            name        = "Power",
            label       = "power",
            min         = 0,
            max         = 1,
            default     = 0
        }
    }
end

local function process (a, m, p)
    local buf = m:get (1)
    local state = p [1]

    output:clear()
    if state ~= laststate then
        -- update the sysex body
        if state == 0.0 then
            bytes.set (sysex, 3, 0x00)
        else
            bytes.set (sysex, 3, 0x01)
        end
        
        -- add sysex to output buffer
        output:addbytes (sysex, 4, 1)
        laststate = state
    end

    buf:swap (output)
end

return {
    type        = 'DSP',
    layout      = layout,
    parameters  = parameters,
    process     = process
}
