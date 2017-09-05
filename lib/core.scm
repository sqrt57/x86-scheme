; Core scheme functionality
;
; This file is part of x86-scheme project.
; Copyright (c) 2015-2017, Dmitry Grigoryev

(builtin-define list (lambda elems elems))

(builtin-define defmacro
  (macro (head . body)
    (list (quote builtin-define)
          (car head)
          (cons (quote macro)
                (cons (cdr head)
                      body)))))

(defmacro (or2 x y)
  (list (list (quote lambda)
              (list (quote or2-x))
              (list (quote if) (quote or2-x) (quote or2-x) y))
        x))

(defmacro (and2 x y)
  (list (quote if) x y #f))

(defmacro (define head . body)
  (if (pair? head)
      (list (quote builtin-define)
            (car head)
            (cons (quote lambda)
                  (cons (cdr head)
                        body)))
      (cons (quote builtin-define) (cons head body))))

(define (caar x) (car (car x)))
(define (cadr x) (car (cdr x)))
(define (cdar x) (cdr (car x)))
(define (cddr x) (cdr (cdr x)))

(define (caaar x) (car (car (car x))))
(define (caadr x) (car (car (cdr x))))
(define (cadar x) (car (cdr (car x))))
(define (caddr x) (car (cdr (cdr x))))
(define (cdaar x) (cdr (car (car x))))
(define (cdadr x) (cdr (car (cdr x))))
(define (cddar x) (cdr (cdr (car x))))
(define (cdddr x) (cdr (cdr (cdr x))))

(define (string-append . strings) (reduce string-append-2 "" strings))
(define (memq obj list)
  (if (pair? list)
      (if (eq? (car list) obj)
          list
          (memq obj (cdr list)))
      #f))

(define *features* (quote (x86-scheme)))

(define (add-feature! feature)
  (if (memq feature *features*)
    (begin)
    (set! *features* (cons feature *features*))))

(define (has-feature? feature)
  (if (memq feature *features*) #t #f))

(define (map f list)
  (if (pair? list)
      (cons (f (car list)) (map f (cdr list)))
      (quote ())))

(define (assq elem list)
  (if (pair? list)
      (if (eq? (caar list) elem)
          (car list)
          (assq elem (cdr list)))
      #f))

(define (alist-copy l)
  (map (lambda (el) (cons (car el) (cdr el))) l))

(define (procedure? obj)
  (or2 (lambda? obj)
       (or2 (native-procedure? obj) (continuation? obj))))

(defmacro (cond . clauses) (fold-right cond-clause #t clauses))

(define (cond-clause clause rest)
  (if (null? (cdr clause))
      ; (c) -> ((lambda (cond-tmp) (if cond-tmp cond-tmp rest)) c)
      (list (list (quote lambda)
                  (quote (cond-tmp))
                  (list (quote if)
                        (quote cond-tmp)
                        (quote cond-tmp)
                        rest))
            (car clause))
      (if (null? (cddr clause))
          ; (c r) -> (if c r rest)
          (list (quote if)
                (car clause)
                (cadr clause)
                rest)
          (error "cond: bad syntax"))))
