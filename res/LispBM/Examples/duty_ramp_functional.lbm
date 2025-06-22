; Ramp motor duty cycle between -1 and 1, functional approach

(def rate 50)      ; Update rate in Hz
(def ramptime 3.0) ; Motor ramp time in seconds

; Step size in each iteration to ramp for
; 3.0 seconds when ramptime is 50 Hz
(def rampstep (/ 1.0 (* rate ramptime)))

(defun f (duty add) {
        (set-duty duty)
        (sleep (/ 1.0 rate))
        
        ; When we make the next call we describe how the arguments, which describe
        ; the state, get updated.
        (f (+ duty add)                                                     ; <- duty
            (if (>= duty 1.0) (- rampstep) (if (<= duty -1.0) rampstep add)) ; <- add
)})

; Start with duty as 0 and add as rampstep
(f 0.0 rampstep)
