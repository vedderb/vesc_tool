(def f (f-open "fws/str500.boot" "r"))
(def can-id 93)

;(def f (f-open "fws/express.boot" "r"))
;(def can-id -1)

(def fwsize (f-size f))

(print "erase")
(print (list "Erase res" (fw-erase (f-size f) can-id)))

(def offset 0)
(loopwhile t {
        (var data (f-read f 256))
        (if (eq data nil) {
                (print "Upload done")
                (break)
        })
        
        (fw-write offset data can-id)
        (setq offset (+ offset (buflen data)))
        (print (list "Progress" (floor (* 100 (/ (to-float offset) fwsize)))))
})

(fw-reboot can-id)
