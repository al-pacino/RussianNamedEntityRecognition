#!/usr/local/bin/python3

__author__ = "Anton Todua"

from os import walk
from os import path
from subprocess import call

texts_path = "../test-texts/"
mystem_path = "../mystem"
mystem_flags = "-ncisd"
main_program_path = "../NamedEntityRecognition"
crf_test_path = "../CRF++-0.58/crf_test" 
crf_test_model_path = "../model.crf-model"

def call_main_program( args, dst_filename ):
	with open( dst_filename, 'wb' ) as file:
		call( [main_program_path] + args, stdout=file )

def save_file_in_cp1251( src_filename, dst_filename ):
	try:
		text = open( src_filename, 'r', encoding='utf-8' ).read()
		text_bytes = text.encode( 'cp1251', errors='replace' )
		open( dst_filename, 'wb' ).write( text_bytes )
	except UnicodeDecodeError:
		print( 'Error: ' + name )

def stem_file( src_filename, dst_filename ):
	if path.isfile( dst_filename ):
		return
	call([mystem_path, mystem_flags, "--eng-gr", "-e", "cp1251",
		"--format", "json", src_filename, dst_filename ])
			
def test_file( src_filename, dst_filename ):
	with open( dst_filename, 'wb' ) as file:
		call([crf_test_path, "-m", crf_test_model_path, src_filename ], \
			stdout=file )

for ( dirpath, dirnames, filenames ) in walk( texts_path ):
	for filename in filenames:
		if filename.endswith( '.txt' ):
			print( filename )
			name = texts_path + filename[:-4]
			save_file_in_cp1251( name + '.txt', name + '.cp1251' )
			stem_file( name + '.cp1251', name + '.json' )
			call_main_program( ['--prepare-test-file', name + '.json'], \
				name + '.signs' )
			test_file( name + '.signs', name + '.crf-tested' )
			call_main_program( ['--prepare-answer-file', name + '.json', \
				name + '.crf-tested'], name + '.task1' )

