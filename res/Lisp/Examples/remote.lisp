(loopwhile t
    (progn
        (define state (get-remote-state))
        (define mote-y (ix state 0))
        (define mote-x (ix state 1))
        (define mote-c (ix state 2))
        (define mote-z (ix state 3))
        (define mote-rev (ix state 4))
        
        (sleep 0.02)
))
