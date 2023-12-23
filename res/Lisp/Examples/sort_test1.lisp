(defun sort2 (f lst)
    (let (
            (insert (fn (elt f sorted-lst)
                    (if (eq sorted-lst nil)
                        (list elt)
                        (if (f elt (car sorted-lst))
                            (cons elt sorted-lst)
                            (cons (car sorted-lst) (insert elt f (cdr sorted-lst)))
            ))))
        )
        (if (eq lst nil)
            nil
            (insert (car lst) f (sort2 f (cdr lst)))
)))

(rand 110) ; seed 110

(def numbers (map (fn (x) (to-i (mod (rand) 1000))) (range 60)))

(defun repeat (f times)
    (if (<= times 1)
        (eval f)
        { (eval f) (repeat f (- times 1))}
))

(defun test () {
        (var ts (systime))
        (repeat '(sort2 < numbers) 50)
        (print (list "Lisp" (secs-since ts)))
        
        (var ts (systime))
        (repeat '(sort < numbers) 50)
        (print (list "Builtin" (secs-since ts)))
})

(spawn 500 test)
