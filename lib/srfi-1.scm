(define (fold-left f z lst)
  (if (pair? lst)
      (fold-left f (f z (car lst)) (cdr lst))
      z))

(define (reduce-left f z lst)
  (if (pair? lst)
      (fold-left f (car lst) (cdr lst))
      z))
