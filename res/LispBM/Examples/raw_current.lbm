(def rate 500) ; Update rate in hz
(def filter-const 0.1)

(defun lpf (val sample)
    (- val (* filter-const (- val sample)))
)

(def i1-f 0)
(def i2-f 0)
(def i3-f 0)

(loopwhile t {
        ; Unfiltered
        (def i1 (raw-adc-current 1 1))
        (def i2 (raw-adc-current 1 2))
        (def i3 (raw-adc-current 1 3))
        
        ; Filtered
        (def i1-f (lpf i1-f (raw-adc-current 1 1)))
        (def i2-f (lpf i2-f (raw-adc-current 1 2)))
        (def i3-f (lpf i3-f (raw-adc-current 1 3)))
        
        (def i-tot (+ i1-f i2-f i3-f)) ; Should sum to 0
        
        (def i1-adc (raw-adc-current 1 1 1))
        (def i2-adc (raw-adc-current 1 2 1))
        (def i3-adc (raw-adc-current 1 3 1))
        
        (sleep (/ 1.0 rate))
})
