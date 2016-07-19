(define b 5)
b
(define b 6)
b
(set! b 7)
b
(define a 1)
(define b 1)
(define q
  (lambda (x)
    (define y (+ x 10))
    (define b 100)
    (set! a 100)
    (+ x y)))
(q 0)
a
b

