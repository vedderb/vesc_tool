(defunret is-sorted (lst) {
        (var prev (ix lst 0))
        
        (loopforeach i lst {
                (if (> prev i) (return false))
                (setq prev i)
        })
        
        t
})

(def pass-cnt 0)

(rand 110) ; seed 110

(def ts (systime))
(looprange i 0 100 {
        (var numbers (map (fn (x) (to-i (mod (rand) 1000))) (range 400)))
        
        (if (not (is-sorted (sort < numbers))) {
                (print "Sort failed")
                (break)
        })
        
        (setq pass-cnt (+ pass-cnt 1))
})

(print (list "Time:" (secs-since ts) "seconds"))

; ESC
; 2023-12-22: 15.85 s
