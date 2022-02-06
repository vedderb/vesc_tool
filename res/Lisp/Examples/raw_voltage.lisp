(define rate 500) ; Update rate in hz
(define filter-const 0.2)

(define filter (lambda (val sample)
    (- val (* filter-const (- val sample)))
))

(define va-f 0)

(define f (lambda ()
    (progn
        (define use-raw 0) ; Set to 1 to use ADC bits and not convert to voltage
    
        (define va (raw-adc-voltage 1 1 use-raw))
        (define vb (raw-adc-voltage 1 2 use-raw))
        (define vc (raw-adc-voltage 1 3 use-raw))
        (define vz (/ (+ va vb vc) 3))
        
        (define va-z (- va vz))
        (define vb-z (- vb vz))
        (define vc-z (- vc vz))
        
        (define va-f (filter va-f va-z))

        (yield (/ 1000000 rate))
        (f)
)))

(f)
