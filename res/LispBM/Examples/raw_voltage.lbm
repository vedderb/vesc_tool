(def rate 500) ; Update rate in hz
(def filter-const 0.2)

(defun lpf (val sample)
    (- val (* filter-const (- val sample)))
)

(def va-f 0)

(loopwhile t {
        (def use-raw 0) ; Set to 1 to use ADC bits and not convert to voltage
        
        (def va (raw-adc-voltage 1 1 use-raw))
        (def vb (raw-adc-voltage 1 2 use-raw))
        (def vc (raw-adc-voltage 1 3 use-raw))
        (def vz (/ (+ va vb vc) 3))
        
        (def va-z (- va vz))
        (def vb-z (- vb vz))
        (def vc-z (- vc vz))
        
        (def va-f (lpf va-f va-z))
        
        (sleep (/ 1.0 rate))
})
