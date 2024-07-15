
; Find the index of the first matching sequencs of seqs
(defun str-find-any (str seqs) (first (sort < (map (fn (x) (to-u (str-find str x))) seqs))))

; Same as above, but works on characters in a string
(defun str-find-any-char (str chars) (str-find-any str (str-split chars 1)))

; Works on both strings and list of strings
(defun str-find-anyx (str seqs)
    (first (sort < (map (fn (x) (to-u (str-find str x)))
        (if (eq (type-of seqs) type-array) (str-split seqs 1) seqs)))
))

; Iterative version that avoids sort
(defun str-find-any-char-iter (str chars) {
        (var tmp "0")

        (bufset-u8 tmp 0 (bufget-u8 chars 0))
        (var res (str-find str tmp))

        (looprange i 1 (str-len chars) {
                (bufset-u8 tmp 0 (bufget-u8 chars i))
                (var occ (str-find str tmp))
                (if (and (>= occ 0) (< occ res)) (setq res occ))
        })

        res
})

(defun repeat (f times)
    (if (<= times 1)
        (eval f)
        { (eval f) (repeat f (- times 1)) }
))

; Speed tests

(def test-str "Testaaaaaaaaaaaaaaaaaaaaaaaaaaaaastring asdasdpiajmsdopijaspdåiojkaspåodkapåoskdpoåaskd")

(def start (systime))
(repeat '(str-find-any-char test-str "åkwp") 1000)
(print (secs-since start))
; Express 2024-07-10 0.445 ms
; Express 2024-07-15 0.741 ms

(def start (systime))
(repeat '(str-find-anyx test-str "åkwp") 1000)
(print (secs-since start))
; Express 2024-07-10 0.467 ms
; Express 2024-07-15 0.789 ms

(def start (systime))
(repeat '(str-find-any test-str '("å" "k" "w" "p")) 1000)
(print (secs-since start))
; Express 2024-07-10 0.275 ms
; Express 2024-07-15 0.497 ms

(def start (systime))
(repeat '(str-find-anyx test-str '("å" "k" "w" "p")) 1000)
(print (secs-since start))
; Express 2024-07-10 0.309 ms
; Express 2024-07-15 0.525 ms

(def start (systime))
(repeat '(str-find-any-char-iter test-str "åkwp") 1000)
(print (secs-since start))
; Express 2024-07-10 0.753 ms
; Express 2024-07-15 1.139 ms

(def start (systime))
(repeat '(str-find test-str '("å" "k" "w" "p")) 1000)
(print (secs-since start))
; Express 2024-07-15 0.185 ms
