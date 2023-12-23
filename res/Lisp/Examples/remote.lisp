(loopwhile t {
        (def state (get-remote-state))
        (def mote-y (ix state 0))
        (def mote-x (ix state 1))
        (def mote-c (ix state 2))
        (def mote-z (ix state 3))
        (def mote-rev (ix state 4))
        
        (sleep 0.02)
})
