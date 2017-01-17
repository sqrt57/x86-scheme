(define builtin-define define)

(define defmacro
  (macro (head . body)
    (list (quote builtin-define)
          (car head)
          (cons (quote macro)
                (cons (cdr head)
                      body)))))

(defmacro (or2 x y)
  ((lambda (x y) (if x x y)) x y))

(defmacro (and2 x y)
  ((lambda (x y) (if x y x)) x y))

(defmacro (define head . body)
  (if (pair? head)
      (list (quote builtin-define)
            (car head)
            (cons (quote lambda)
                  (cons (cdr head)
                        body)))
      (cons (quote builtin-define) (cons head body))))
