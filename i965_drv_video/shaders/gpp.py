#!/usr/bin/env python
#coding=UTF-8

# Copyright © 2011 Intel Corporation
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice (including the next
# paragraph) shall be included in all copies or substantial portions of the
# Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#
# Authors:
#    Chen, Yangyang <yangyang.chen@intel.com>
#    Han, Haofu     <haofu.han@intel.com>
#

import sys

class Block:
	def __init__(self, ln=0, s=None):
		assert type(ln) == int
		assert type(s) == str or s == None
		self.lineno = ln
		self.text = s
		self.subblocks = []

	def append(self, block):
		self.subblocks.append(block)

	def checkfor(self, line):
		import re
		p = r'\$\s*for\s*'
		if re.match(p, line) == None:
			raise Exception(self.__errmsg('syntax error'))
		tail = line.split('(', 1)[1].rsplit(')', 1)
		conds = tail[0].split(';')
		lb = tail[1]
		if lb.strip() != '{':
			raise Exception(self.__errmsg('missing "{"'))
		if len(conds) != 3:
			raise Exception(self.__errmsg('syntax error(miss ";"?)'))
		init = conds[0]
		cond = conds[1]
		step = conds[2]
		self.__parse_init(init)
		self.__parse_cond(cond)
		self.__parse_step(step)

	def __parse_init(self, init):
		inits = init.split(',')
		self.param_init = []
		for ini in inits:
			try:
				val = eval(ini)
				self.param_init.append(val)
			except:
				raise Exception(self.__errmsg('non an exp: %s'%ini))
		self.param_num = len(inits)

	def __parse_cond(self, cond):
		cond = cond.strip()
		if cond[0] in ['<', '>']:
			if cond[1] == '=':
				self.param_op = cond[:2]
				limit = cond[2:]
			else:
				self.param_op = cond[0]
				limit = cond[1:]
			try:
				self.param_limit = eval(limit)
			except:
				raise Exception(self.__errmsg('non an exp: %s'%limit))
		else:
			raise Exception(self.__errmsg('syntax error'))

	def __parse_step(self, step):
		steps = step.split(',')
		if len(steps) != self.param_num:
			raise Exception(self.__errmsg('params number no match'))
		self.param_step = []
		for st in steps:
			try:
				val = eval(st)
				self.param_step.append(val)
			except:
				raise Exception(self.__errmsg('non an exp: %s'%st))

	def __errmsg(self, msg=''):
		return '%d: %s' % (self.lineno, msg)

def readlines(f):
	lines = f.readlines()
	buf = []
	for line in lines:
		if '\\n' in line:
			tmp = line.split('\\n')
			buf.extend(tmp)
		else:
			buf.append(line)
	return buf

def parselines(lines):
	root = Block(0)
	stack = [root]
	lineno = 0
	for line in lines:
		lineno += 1
		line = line.strip()
		if line.startswith('$'):
			block = Block(lineno)
			block.checkfor(line)
			stack[-1].append(block)
			stack.append(block)
		elif line.startswith('}'):
			stack.pop()
		elif line and not line.startswith('#'):
			stack[-1].append(Block(lineno, line))
	return root

def writeblocks(outfile, blocks):
	buf = []

	def check_cond(op, cur, lim):
		assert op in ['<', '>', '<=', '>=']
		assert type(cur) == int
		assert type(lim) == int
		return eval('%d %s %d' % (cur, op, lim))

	def do_writeblock(block, curs):
		if block.text != None:
			import re
			p = r'\%(\d+)'
			newline = block.text
			params = set(re.findall(p, block.text))
			for param in params:
				index = int(param) - 1
				if index >= len(curs):
					raise Exception('%d: too many param(%%%d)'%(block.lineno, index+1))
				newline = newline.replace('%%%d'%(index+1), str(curs[index]))
			if newline and \
					not newline.startswith('.') and \
					not newline.endswith(':') and \
					not newline.endswith(';'):
				newline += ';'
			buf.append(newline)
		else:
			for_curs = block.param_init
			while check_cond(block.param_op, for_curs[0], block.param_limit):
				for sblock in block.subblocks:
					do_writeblock(sblock, for_curs)
				for i in range(0, block.param_num):
					for_curs[i] += block.param_step[i]

	for block in blocks.subblocks:
		do_writeblock(block, [])
	outfile.write('\n'.join(buf))
	outfile.write('\n')

if __name__ == '__main__':
	argc = len(sys.argv)
	if argc == 1:
		print >>sys.stderr, 'no input file'
		sys.exit(0)

	try:
		infile = open(sys.argv[1], 'r')
	except IOError:
		print >>sys.stderr, 'can not open %s' % sys.argv[1]
		sys.exit(1)

	if argc == 2:
		outfile = sys.stdout
	else:
		try:
			outfile = open(sys.argv[2], 'w')
		except IOError:
			print >>sys.stderr, 'can not write to %s' % sys.argv[2]
			sys.exit(1)

	lines = readlines(infile)
	try:
		infile.close()
	except IOError:
		pass

	blocks = parselines(lines)
	writeblocks(outfile, blocks)
