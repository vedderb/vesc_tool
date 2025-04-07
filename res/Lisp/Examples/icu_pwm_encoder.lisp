(icu-start 1000000 1)

(defun min (a b) (if (> a b) b a))

(loopwhile t {
        (if (> (icu-period) 0) {
                (var period (to-float (icu-period)))
                (def angle (* (/ (min (icu-width) period) period) 360.0))
                ;(def agle-abi (get-encoder))
                (set-encoder angle)
        })

        (sleep 0.0005)
})