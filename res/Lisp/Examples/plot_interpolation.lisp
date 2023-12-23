; Interpolation and plotting test. Go the the experiment plot to see the results while running this example.

; See http://paulbourke.net/miscellaneous/interpolation/

(defun fun-linear (y0 y1 y2 y3 mu)
    (+ (* y1 (- 1 mu)) (* y2 mu))
)

(defun fun-cos (y0 y1 y2 y3 mu) {
        (var mu2 (/ (- 1.0 (cos (* mu 3.1415923))) 2.0))
        (+ (* y1 (- 1.0 mu2)) (* y2 mu2))
})

(defun fun-cubic (y0 y1 y2 y3 mu) {
        (var mu2 (* mu mu))
        (var a0 (+ (- y3 y2 y0) y1))
        (var a1 (- y0 y1 a0))
        (var a2 (- y2 y0))
        
        (+ (* a0 mu mu2) (* a1 mu2) (* a2 mu) y1)
})

(defun fun-hermite (y0 y1 y2 y3 mu) {
        (var mu2 (* mu mu))
        (var a0 (+ (* -0.5 y0) (* 1.5 y1) (* -1.5 y2) (* 0.5 y3)))
        (var a1 (+ y0 (* -2.5 y1) (* 2.0 y2) (* -0.5 y3)))
        (var a2 (+ (* -0.5 y0) (* 0.5 y2)))
        
        (+ (* a0 mu mu2) (* a1 mu2) (* a2 mu) y1)
})

(defun trunc (val min max)
    (cond
        ((< val min) min)
        ((> val max) max)
        (t val)
))

(defun interpolate (val tab fun)
    (let (
            (ind (looprange i 0 (length tab)
                    (if (> (first (ix tab i)) val)
                        (break i)
                        i
            )))
            (last-ind (- (length tab) 1))
            (y0 (second (ix tab (trunc (- ind 2) 0 last-ind))))
            (y1 (second (ix tab (trunc (- ind 1) 0 last-ind))))
            (y2 (second (ix tab ind)))
            (y3 (second (ix tab (trunc (+ ind 1) 0 last-ind))))
            (p0 (first (ix tab (trunc (- ind 1) 0 last-ind))))
            (p1 (first (ix tab ind)))
            (mu (if (= ind 0)
                    0.0
                    (trunc (/ (- val p0) (- p1 p0)) 0.0 1.0)
            ))
        )
        
        (fun y0 y1 y2 y3 mu)
))

(defun plot-range (start end points tab fun)
    (looprange i 0 points {
            (var val (+ (* (/ i (to-float points)) (- end start)) start))
            (plot-send-points val (interpolate val tab fun))
    })
)

(def int-tab '(
        (-500.0 90.0)
        (0.0 80.0)
        (500.0 82.0)
        (1000.0 90.0)
        (1500.0 94.0)
        (3000.0 96.0)
        (5000.0 100.0)
))

(def val-start -1000.0)
(def val-end 5500.0)
(def points 300)

(plot-init "Val" "Output")
(plot-add-graph "Linear")
(plot-add-graph "Cosine")
(plot-add-graph "Cubic")
(plot-add-graph "Hermite")

(plot-set-graph 0)
(plot-range val-start val-end points int-tab fun-linear)

(plot-set-graph 1)
(plot-range val-start val-end points int-tab fun-cos)

(plot-set-graph 2)
(plot-range val-start val-end points int-tab fun-cubic)

(plot-set-graph 3)
(plot-range val-start val-end points int-tab fun-hermite)

; ---------- Performance Tests ------------- ;

; Measure the time it takes to run expr in milliseconds.
; Repeat times and take average.
(defun time-expr-ms (expr times) {
        (var seq (append '(progn) (map (fn (x) expr) (range times))))
        (var start (systime))
        (var res (eval seq))
        
        (* (/ (secs-since start) times) 1000)
})

(print (str-from-n (time-expr-ms '(interpolate -600 int-tab fun-linear) 50) "\nLinear start : %.2f ms"))
(print (str-from-n (time-expr-ms '(interpolate 1100 int-tab fun-linear) 50) "Linear mid   : %.2f ms"))
(print (str-from-n (time-expr-ms '(interpolate 6000 int-tab fun-linear) 50) "Linear end   : %.2f ms\n\n"))

(print (str-from-n (time-expr-ms '(interpolate -600 int-tab fun-cos) 50) "Cosine start : %.2f ms"))
(print (str-from-n (time-expr-ms '(interpolate 1100 int-tab fun-cos) 50) "Cosine mid   : %.2f ms"))
(print (str-from-n (time-expr-ms '(interpolate 6000 int-tab fun-cos) 50) "Cosine end   : %.2f ms\n\n"))

(print (str-from-n (time-expr-ms '(interpolate -600 int-tab fun-cubic) 50) "Cubic start  : %.2f ms"))
(print (str-from-n (time-expr-ms '(interpolate 1100 int-tab fun-cubic) 50) "Cubic mid    : %.2f ms"))
(print (str-from-n (time-expr-ms '(interpolate 6000 int-tab fun-cubic) 50) "Cubic end    : %.2f ms\n\n"))

(print (str-from-n (time-expr-ms '(interpolate -600 int-tab fun-hermite) 50) "Hermite start: %.2f ms"))
(print (str-from-n (time-expr-ms '(interpolate 1100 int-tab fun-hermite) 50) "Hermite mid  : %.2f ms"))
(print (str-from-n (time-expr-ms '(interpolate 6000 int-tab fun-hermite) 50) "Hermite end  : %.2f ms\n\n"))

;-- Results 2022-12-28

;Linear start : 2.40 ms
;Linear mid   : 3.26 ms
;Linear end   : 3.64 ms
;
;Cosine start : 2.46 ms
;Cosine mid   : 3.37 ms
;Cosine end   : 3.71 ms
;
;Cubic start  : 2.60 ms
;Cubic mid    : 3.47 ms
;Cubic end    : 3.76 ms
;
;Hermite start: 2.77 ms
;Hermite mid  : 3.64 ms
;Hermite end  : 3.93 ms
