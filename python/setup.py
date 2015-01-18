#!/usr/bin/python
# -*- coding: utf-8 -*-

from distutils.core import setup, Extension

setup(
	name = 'transact',
	version = '1.0',
	author = 'Luis Héctor Chávez',
	author_email = 'lhchavez@omegaup.com',
	description = (
			'A super fast synchronous IPC mechanism over shm with transact as '
			'signalling method'),
	ext_modules = [Extension('transact', sources=['transactmodule.c'])])
