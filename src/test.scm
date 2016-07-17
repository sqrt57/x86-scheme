(define b 5)
(define q (lambda (x) (+ x 10)))
(q b)
(define w (lambda (x) (define y (+ x 10)) (+ x y)))
(w 3)

