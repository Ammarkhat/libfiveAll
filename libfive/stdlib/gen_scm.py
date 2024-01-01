#!/usr/bin/env python3

import os
from datetime import datetime

import parse

def arg_type(a):
    return {'libfive_tree': "'*",
            'tfloat': "'*",
            'tvec2':  "(list '* '*)",
            'tvec3':  "(list '* '* '*)",
            'float': 'float',
            'const char*': "'*",
            'int':   'int'}[a.type]

def arg_name(args, i):
    a = args[i]
    if a.default:
        if a.type == 'tfloat':
            d = str(a.default)
        elif a.type == 'tvec2':
            d = "#[{0} {0}]".format(a.default)
        elif a.type == 'tvec3':
            d = "#[{0} {0} {0}]".format(a.default)
        else:
            raise RuntimeError("Unknown default: {}".format(a))
        d = "({} {})".format(a.name, d)

        if i == 0 or not args[i - 1].default:
            d = "#:optional " + d
        return d
    else:
        return a.name

def arg_call(a):
    if a.type in ['libfive_tree', 'tfloat']:
        return '(shape->ptr (ensure-shape {}))'.format(a.name)
    elif a.type in ['float', 'int']:
        return a.name
    elif a.type == 'tvec2':
        return "(vec2->tvec2 {})".format(a.name)
    elif a.type == 'tvec3':
        return "(vec3->tvec3 {})".format(a.name)
    elif a.type == 'const char*':
        return "(string->pointer {})".format(a.name)
    else:
        raise RuntimeError("Unknown type %s" % a.type)

def format_module(lib, m):
    out = '''#|
Guile bindings to the libfive CAD kernel

DO NOT EDIT BY HAND!
This file is automatically generated from libfive/stdlib/libfive_stdlib.h

It was last generated on {} by user {}
|#

(define-module (libfive stdlib {}))
(use-modules (system foreign) (libfive lib) (libfive kernel) (libfive vec))

'''.format(datetime.now().strftime("%Y-%m-%d %H:%M:%S"), os.getlogin(), m)

    for f in lib[m].shapes:
        arg_types = " ".join(map(arg_type, f.args))
        arg_names = " ".join([arg_name(f.args, i) for i in range(0, len(f.args))])
        arg_calls = "\n    ".join(map(arg_call, f.args))
        name = f.name.replace('_', '-')
        out += '''(define ffi_{name} (pointer->procedure '*
  (dynamic-func "{raw_name}" stdlib)
  (list {arg_types})))
(define* ({name} {arg_names})
  "{name}{s}{arg_names}
  {doc}
  (ptr->shape (ffi_{name}
    {arg_calls})))
(export {name})

'''.format(raw_name=f.raw_name or f.name,
       name=name,
       s=' ' if f.args else '',
       doc=f.docstring.replace('\n', '\n  ') + '"',
       arg_types=arg_types,
       arg_names=arg_names,
       arg_calls=arg_calls)

    for a in lib[m].aliases:
        out += '''(define-public {} {})
'''.format(a.name.replace('_', '-'), a.target.replace('_', '-'))
    return out[:-1]

################################################################################

append = {'csg':
'''
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Hand-written functions which allow for arbitrary numbers of arguments
(use-modules (srfi srfi-1))

(define union-prev union)
(define-public (union a . args)
  "union a [b [c [...]]]
  Returns the union of any number of shapes"
  (fold union-prev a args))

(define intersection-prev intersection)
(define-public (intersection a . args)
  "intersection a [b [c [...]]]
  Returns the intersection of any number of shapes"
  (fold intersection-prev a args))

(define-public (difference a . bs)
  "difference a b [c [d [...]]]
  Subtracts any number of shapes from the first argument"
  (intersection a (inverse (apply union bs))))
''',

'transforms':
'''
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Hand-written override to let move work with vec2
(define move-prev move)
(define-public (move shape v)
  (if (vec2? v) (move-prev shape #[(.x v) (.y v) 0])
                (move-prev shape v)))
(set-procedure-property! move 'documentation
  (procedure-documentation move-prev))
''',

'shapes':'''
(define-public pi 3.14159265359)'''
}

################################################################################


stdlib = parse.parse_stdlib()
for m in ['csg', 'shapes', 'transforms', 'text']:
    with open('../bind/guile/libfive/stdlib/%s.scm' % m, 'w') as f:
        f.write(format_module(stdlib, m))
        if m in append:
            f.write(append[m])
