; This makes a plot on the Realtime Data -> Experiment page

(plot-init "x-name" "y-name")
(plot-add-graph "sin")
(plot-add-graph "cos")

(plot-set-graph 0)
(looprange i 0 200
    (plot-send-points (/ i 10.0) (sin (/ i 10.0)))
)

(plot-set-graph 1)
(looprange i 0 200
    (plot-send-points (/ i 10.0) (cos (/ i 10.0)))
)
