(def rate 200) ; Update rate in hz

(loopwhile t {
        (def va (raw-adc-voltage 1 1))
        (def vb (raw-adc-voltage 1 2))
        (def vc (raw-adc-voltage 1 3))
        (def vz (/ (+ va vb vc) 3))
        
        ; The meaured phase voltages with their offset removed
        (def va-no-ofs (- va vz))
        (def vb-no-ofs (- vb vz))
        (def vc-no-ofs (- vc vz))
        
        ; Calculate phase voltages by taking the inverse Clarke transform
        ; of the modulation and multiplying that with v_in / (3 / 2)
        (def sqrt-3 1.732050807568877)
        (def va-calc (* (/ (get-vin) 1.5) (raw-mod-alpha)))
        (def vb-calc (* (/ (get-vin) 1.5) (/ (+ (- (raw-mod-alpha)) (+ (* sqrt-3 (raw-mod-beta))) ) 2)))
        (def vc-calc (* (/ (get-vin) 1.5) (/ (+ (- (raw-mod-alpha)) (- (* sqrt-3 (raw-mod-beta))) ) 2)))
        
        (sleep (/ 1.0 rate))
})
