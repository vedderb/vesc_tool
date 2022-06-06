(define rate 500) ; Update rate in hz
(define filter-const 0.1)

(define filter (lambda (val sample)
    (- val (* filter-const (- val sample)))
))

(define i1-f 0)
(define i2-f 0)
(define i3-f 0)

(loopwhile t
    (progn
        ; Unfiltered
        (define i1 (raw-adc-current 1 1))
        (define i2 (raw-adc-current 1 2))
        (define i3 (raw-adc-current 1 3))
       
        ; Filtered
        (define i1-f (filter i1-f (raw-adc-current 1 1)))
        (define i2-f (filter i2-f (raw-adc-current 1 2)))
        (define i3-f (filter i3-f (raw-adc-current 1 3)))
        
        (define i-tot (+ i1-f i2-f i3-f)) ; Should sum to 0
        
        (define i1-adc (raw-adc-current 1 1 1))
        (define i2-adc (raw-adc-current 1 2 1))
        (define i3-adc (raw-adc-current 1 3 1))

        (sleep (/ 1.0 rate))
))
