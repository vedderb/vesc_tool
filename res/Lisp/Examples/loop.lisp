(define rate 50) ; Update rate in hz

(define f (lambda ()
    (progn
        ; Put your code here

        (yield (/ 1000000 rate))
        (f)
)))

(f)
