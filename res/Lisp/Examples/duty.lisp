; This program will ramp the duty cycle up to 1 and then
; down to -1 repeatedly according to the parameters below.

(define rate 50)      ; Update rate in Hz
(define ramptime 3.0) ; Motor ramp time in seconds

; Step for each iteration based on rate and ramptime
(define rampstep (/ 1.0 (* rate ramptime)))

; duty is the duty cycle now and add is how much to add to
; it for the next iteration. When duty gets larger than 1 we
; make add negative and when it gets smaller than -1 we make
; add positive. When -1 < add < 1 the previos value of add
; is used.
(defun f (duty add)
    (progn
        (set-duty duty)
        (sleep (/ 1.0 rate))
     
        (f (+ duty add)
           (if (>= duty 1.0)
                (- rampstep)
                (if (<= duty -1.0)
                    rampstep
                    add
)))))

; Start with duty cycle 0 and add as the positive rampstep
(f 0 rampstep)
