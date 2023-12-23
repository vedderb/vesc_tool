; Sin/Cos Encoder calibration helper.

; How to use
; 1. Spin the motor slowly until s-min, s-max, c-min and c-max don't change any more
; 2. The amplitudes and offsets are now in s-amp, s-ofs, c-amp and c-ofs
; 3. Go to FOC->Encoder and enter the calculated amplitudes and offsets

(def raw-sin (get-adc 0))
(def raw-cos (get-adc 1))

(def out-sin 0.0)
(def out-cos 0.0)
(def out-mod 0.0)

(def s-amp 1.0)
(def c-amp 1.0)

(def s-ofs 1.65)
(def c-ofs 1.65)

(defun reset-max-min () {
        (def s-max 0.0)
        (def s-min 5.0)
        (def c-max 0.0)
        (def c-min 5.0)
})

(reset-max-min)

(defun calc-amp-ofs (print-res) {
        (setq s-ofs (/ (+ s-max s-min) 2.0))
        (setq c-ofs (/ (+ c-max c-min) 2.0))
        (setq s-amp (/ (- s-max s-min) 2.0))
        (setq c-amp (/ (- c-max c-min) 2.0))
        
        (if print-res {
                (print (str-merge (str-from-n s-amp "Sin Amp: %.3f ") (str-from-n s-ofs "Sin Offset: %.3f ")))
                (print (str-merge (str-from-n c-amp "Cos Amp: %.3f ") (str-from-n c-ofs "Cos Offset: %.3f ")))
        })
})

(def filter-const 0.8)
(defun lpf (val sample)
    (- val (* filter-const (- val sample)))
)

(loopwhile t {
        (def raw-sin (lpf raw-sin (get-adc 0)))
        (def raw-cos (lpf raw-cos (get-adc 1)))
        
        (if (> raw-sin s-max) (def s-max raw-sin))
        (if (< raw-sin s-min) (def s-min raw-sin))
        (if (> raw-cos c-max) (def c-max raw-cos))
        (if (< raw-cos c-min) (def c-min raw-cos))
        
        (def out-sin (/ (- raw-sin s-ofs) s-amp))
        (def out-cos (/ (- raw-cos c-ofs) c-amp))
        (def out-mod (sqrt (+ (* out-sin out-sin) (* out-cos out-cos))))
        
        (if (and (> (- s-max s-min) 0.1) (> (- c-max c-min) 0.1))
            (calc-amp-ofs false)
        )
        
        (sleep 0.01)
})
