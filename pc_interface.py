#!/usr/bin/python3

import json
import requests
from appJar import gui

window = gui('IoT Button Notifier')

url = None
# ----------------------------------------- Functions and callbacks


def checkEverySecond():
    # returns True, False, or None
    global url, window, requests, acknowledge

    if url is None:
        return
    try:
        resp = requests.get(url)
    except Exception as e:
        print(e)
        window.setEntryInvalid('addr')
        return

    window.setEntryValid('addr')

    if resp.status_code == 200:
        # no button yet, but connection is ok, so return
        return

    elif resp.status_code == 201:
        # there is a button press
        # so alert the user, and bring up the menu if not already

        if window.popUp('Button Press',
                        'A user has pressed the button, do you want to set a message?',
                        'question'):
            window.show()

        acknowledge()  # clear the flag on the ESP


def sendMsg(line1, line2, opt):
    global url, requests

    if url is None:
        return

    blink_value = 0

    if opt == 'slow':
        blink_value = 1000
    elif opt == 'fast':
        blink_value = 500

    params = {
        # quote the string so that no illegal characters get through
        'line1': line1,
        'line2': line2,
        'opt': blink_value,
    }

    resp = requests.post(url + '/lcd', data=params)
    print(resp)

    if resp.status_code != 200:
        return False  # failed for some reason
    return True


def acknowledge():
    global url, window
    if url is None:
        return

    resp = requests.post(url + "/acknowledge")

    if resp.status_code != 200:
        window.popUp('fail', 'failed to acknowledge button', 'ok')


def buttonPress(button):
    global window, sendMsg
    if button == 'Send':
        print('sending message to unit')

        line1 = window.getEntry('line1').strip()
        line2 = window.getEntry('line2').strip()
        opt = window.getOptionBox('Blink')

        if sendMsg(line1, line2, opt):
            print('message sent succesfully')
        else:
            print('message failed to send')

    # you can add more buttons here
    # and add callback functions
    # when their name is used


def addrChange(entry):
    global url, window
    addr = window.getEntry('addr')
    print('changing addr to ', addr)
    if addr == '':
        url = None
        return

    url = 'http://' + addr.strip()


def lineChange(e):
    if e != 'line1' and e != 'line2':
        return

    global window

    if len(window.getEntry(e)) > 15:
        if e == 'line1':
            # change focus, move character to other line,
            window.setEntryFocus('line2')
        else:
            # focused on line 2, trim the text always to be 15 (+1)
            window.setEntry('line2', window.getEntry('line2')[:15])


# ----------------------------------------- GUI Options
# we build the window, attach the functions from above, and .go()

window.addLabel('Device IP:')
window.addValidationEntry('addr')
window.addLabel('Current Message:')

window.addLabelEntry('line1')
window.addLabelEntry('line2')
window.setEntryMaxLength('line1', 16)
window.setEntryMaxLength('line2', 16)
window.getEntryWidget('line1').config(font='monospace 12')
window.getEntryWidget('line2').config(font='monospace 12')

window.addOptionBox('Blink', ['no blink', 'slow', 'fast'])

window.addButton('Send', buttonPress)
window.setEntrySubmitFunction('addr', addrChange)
window.setEntryChangeFunction('line1', lineChange)
window.setEntryChangeFunction('line2', lineChange)
# run this continously in the background
window.registerEvent(checkEverySecond)

window.go()
