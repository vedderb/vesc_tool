; Sin/Cos Encoder calibration helper.

; How to use
; 1. Spin the motor slowly until s-min, s-max, c-min and c-max don't change any more
; 2. The gains and offsets are now in s-gain, s-ofs, c-gain and c-ofs
; 3. Go to FOC->Encoder and enter the calculated gains and offsets

(def raw-sin (get-adc 0))
(def raw-cos (get-adc 1))

(def out-sin 0.0)
(def out-cos 0.0)
(def out-mod 0.0)

(def s-gain 1.0)
(def c-gain 1.0)

(def s-ofs 1.65)
(def c-ofs 1.65)

(defun reset-max-min ()
    (progn
        (def s-max 0.0)
        (def s-min 5.0)
        (def c-max 0.0)
        (def c-min 5.0)
))

(reset-max-min)

(defun calc-gain-ofs (print-res)
    (progn
        (def s-ofs (/ (+ s-max s-min) 2.0))
        (def c-ofs (/ (+ c-max c-min) 2.0))
        (def s-gain (* 2.0 (/ 1.0 (- s-max s-min))))
        (def c-gain (* 2.0 (/ 1.0 (- c-max c-min))))
        (if print-res
            (progn
                (print (str-merge (str-from-n s-gain "Sin Gain: %.2f ") (str-from-n s-ofs "Sin Offset: %.2f ")))
                (print (str-merge (str-from-n c-gain "Cos Gain: %.2f ") (str-from-n c-ofs "Cos Offset: %.2f ")))
))))

(def filter-const 0.1)
(defun lpf (val sample)
    (- val (* filter-const (- val sample)))
)

(loopwhile t
    (progn
        (def raw-sin (lpf raw-sin (get-adc 0)))
        (def raw-cos (lpf raw-cos (get-adc 1)))
        
        (if (> raw-sin s-max) (def s-max raw-sin))
        (if (< raw-sin s-min) (def s-min raw-sin))
        (if (> raw-cos c-max) (def c-max raw-cos))
        (if (< raw-cos c-min) (def c-min raw-cos))
        
        (def out-sin (* (- raw-sin s-ofs) s-gain))
        (def out-cos (* (- raw-cos c-ofs) c-gain))
        (def out-mod (sqrt (+ (* out-sin out-sin) (* out-cos out-cos))))
        
        (if (and (> (- s-max s-min) 0.1) (> (- c-max c-min) 0.1))
            (calc-gain-ofs false)
        )

        (sleep 0.01)
))
