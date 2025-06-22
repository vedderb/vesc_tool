; These files can be generated using the VESC Tool CLI

;(def f (f-open "lbm/test_native.lpkg" "r"))
;(def can-id 26)

(def f (f-open "lbm/test.lpkg" "r"))
(def can-id -1)

(def fsize (f-size f))

(print "erase")
(print (list "Erase res" (lbm-erase can-id)))

(def offset 0)
(loopwhile t {
        (var data (f-read f 256))
        (if (eq data nil) {
                (print "Upload done")
                (break)
        })
        
        (lbm-write offset data can-id)
        (setq offset (+ offset (buflen data)))
        (print (list "Progress" (floor (* 100 (/ (to-float offset) fsize)))))
})

(print (list "Run res" (lbm-run 1 can-id)))
