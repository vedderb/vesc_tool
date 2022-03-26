; Ramp motor duty cycle between -1 and 1, imperative approach

(define rate 50)      ; Update rate in Hz
(define ramptime 3.0) ; Motor ramp time in seconds

; Step size in each iteration to ramp for
; 3.0 seconds when ramptime is 50 Hz
(define rampstep (/ 1.0 (* rate ramptime)))

; Create and initialize state that we are going to
; mutate as the program runs.
(define add rampstep)
(define duty 0)

(loopwhile t
    (progn
        (set-duty duty)
        (sleep (/ 1.0 rate))

        ; Update state
        (define duty (+ duty add))
        (define add (if (>= duty 1.0) (- rampstep) (if (<= duty -1.0) rampstep add)))
))
