; Calculate motor temperature from resistance observer. Works best on low speed and high current.

(def tfac 0.00386)
(def r-comp 0.004) ; Sum of cable and MOSFET resistance
(def r-base (- (* 0.001 (conf-get 'foc-motor-r)) r-comp))
(def t-base 25.0) ; Temperature at which the motor resistance was measured

(loopwhile t {
        (def t-meas (get-temp-mot))
        (def r-est (- (get-est-res) r-comp))
        (def t-est (+ t-base (/ (- r-est r-base) (* tfac r-base))))
        (sleep 0.1)
})
