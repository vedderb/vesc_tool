; Ramp motor duty cycle between -1 and 1, imperative approach

(def rate 50)      ; Update rate in Hz
(def ramptime 3.0) ; Motor ramp time in seconds

; Step size in each iteration to ramp for
; 3.0 seconds when ramptime is 50 Hz
(def rampstep (/ 1.0 (* rate ramptime)))

; Create and initialize state that we are going to
; mutate as the program runs.
(def add rampstep)
(def duty 0)

(loopwhile t {
        (set-duty duty)
        (sleep (/ 1.0 rate))
        
        ; Update state
        (setq duty (+ duty add))
        (setq add (if (>= duty 1.0) (- rampstep) (if (<= duty -1.0) rampstep add)))
})
