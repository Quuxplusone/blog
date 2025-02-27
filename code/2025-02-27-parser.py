
PREC = {
  k: v for v, ks in enumerate([
    ['||'], ['&&'], ['|'], ['^'], ['&'],
    ['==', '!='], ['<', '<=', '>', '>='], ['<=>'],
    ['<<', '>>'], ['+', '-'], ['*', '/', '%'],
    ['.*', '->*'],
    ['U'+t for t in ['+', '-', '!', '~', '*', '&']],
    ['.', '->'],
  ]) for k in ks
}
PREC['('] = -float('inf')

class Lexer:
  def tokenize(self, chars):
    self.token = ''
    for ch in chars:
      if ch.isspace():
        yield from self.emit_if(True)
      elif ch.isalnum():
        yield from self.emit_if(not self.token.isalnum())
        self.token += ch
      else:
        yield from self.emit_if(self.token.isalnum())
        yield from self.emit_if((self.token + ch) not in PREC.keys())
        self.token += ch
    yield from self.emit_if(True)

  def emit_if(self, b):
    if b and self.token:
      yield self.token
      self.token = ''

UNPROBLEMATIC = sum([
  [('||',r) for r in ['||', '==', '!=', '<', '<=', '>', '>=', '+', '-', '*', '/', '%']],
  [('&&',r) for r in ['&&', '==', '!=', '<', '<=', '>', '>=', '+', '-', '*', '/', '%']],
  [(x,x) for x in '|^&'],
  [(l,r) for l in ['==', '!=', '<', '<=', '>', '>='] for r in ['||', '&&', '+', '-', '*', '/', '%']],
  [(l,r) for l in ['+', '-', '*'] for r in ['||', '&&', '==', '!=', '<', '<=', '>', '>=', '+', '-', '*', '/', '%']],
  [('/',r) for r in ['||', '&&', '==', '!=', '<', '<=', '>', '>=', '+', '-']],
  [('%',r) for r in ['||', '&&', '==', '!=', '<', '<=', '>', '>=']],
  [(l,r) for l in ['U+', 'U-', 'U~', 'U*', 'U&'] for r in ['||', '&&', '|', '^', '&', '==', '!=', '<', '<=', '>', '>=', '<=>', '<<', '>>', '+', '-', '*', '/', '%']],
], [])

def has_higher_precedence(a, b):
  if a in ['==', '!='] and (a == b):
    print('warning: (x %s y %s z) doesn\'t mean what you think' % (a, b))
  elif a in ['<', '<='] and b in ['<', '<=']:
    print('warning: (x %s y %s z) doesn\'t mean what you think' % (a, b))
  elif a in ['>', '>='] and b in ['>', '>=']:
    print('warning: (x %s y %s z) doesn\'t mean what you think' % (a, b))
  elif a[0] == 'U' and b in ['.*', '->*']:
    print('warning: (%sx%sy) means (%sx)%sy' % (a[1:], b, a[1:], b))
  elif a in ['(', '.', '->', '.*', '->*'] or b in ['.', '->', '.*', '->*']:
    pass # not problematic
  elif (a,b) in UNPROBLEMATIC:
    pass # not problematic
  elif a[0] == 'U':
    print('warning: (%sx %s y) is ambiguous; consider adding parentheses' % (a[1:], b))
  else:
    print('warning: (x %s y %s z) is ambiguous; consider adding parentheses' % (a, b))
  return PREC[a] >= PREC[b]

# https://en.wikipedia.org/wiki/Shunting_yard_algorithm
def infix_to_postfix(tokens):
  stack = []
  expect_primary = True
  for token in tokens:
    if token.isalnum():
      if not expect_primary:
        raise ValueError('got "%s" when a binary operator was expected' % token)
      expect_primary = False
      yield token
    elif token == '(':
      if not expect_primary:
        raise ValueError('got "(" when a binary operator was expected')
      stack.append(token)
    elif token == ')':
      if expect_primary:
        raise ValueError('got ")" when a primary-expression was expected')
      while stack and stack[-1] != '(':
        yield stack.pop()
      if not stack:
        raise ValueError('right parenthesis with no preceding left parenthesis')
      stack.pop()
    elif expect_primary:
      if ('U' + token) not in PREC:
        raise ValueError('unknown unary operator "%s"' % token)
      stack.append('U' + token)
    else:
      if token not in PREC:
        raise ValueError('unknown binary operator "%s"' % token)
      while stack and has_higher_precedence(stack[-1], token):
        yield stack.pop()
      stack.append(token)
      expect_primary = True
  while stack:
    if stack[-1] == '(':
      raise ValueError('left parenthesis with no matching right parenthesis')
    yield stack.pop()

while True:
  line = input()
  tokens = Lexer().tokenize(line)
  try:
    print(' '.join(infix_to_postfix(tokens)))
  except ValueError as e:
    print(e)
