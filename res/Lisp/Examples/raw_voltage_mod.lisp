(define rate 200) ; Update rate in hz

(loopwhile t
    (progn
        (define va (raw-adc-voltage 1 1))
        (define vb (raw-adc-voltage 1 2))
        (define vc (raw-adc-voltage 1 3))
        (define vz (/ (+ va vb vc) 3))
        
        ; The meaured phase voltages with their offset removed
        (define va-no-ofs (- va vz))
        (define vb-no-ofs (- vb vz))
        (define vc-no-ofs (- vc vz))
        
        ; Calculate phase voltages by taking the inverse Clarke transform
        ; of the modulation and multiplying that with v_in / (3 / 2)
        (define sqrt-3 1.732050807568877)
        (define va-calc (* (/ (get-vin) 1.5) (raw-mod-alpha)))
        (define vb-calc (* (/ (get-vin) 1.5) (/ (+ (- (raw-mod-alpha)) (+ (* sqrt-3 (raw-mod-beta))) ) 2)))
        (define vc-calc (* (/ (get-vin) 1.5) (/ (+ (- (raw-mod-alpha)) (- (* sqrt-3 (raw-mod-beta))) ) 2)))

        (sleep (/ 1.0 rate))
))
