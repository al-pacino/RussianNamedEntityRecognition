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

def call_main_program( args, output_filename ):
	with open( output_filename, 'wb' ) as file:
		call( [main_program_path] + args, stdout=file )

def save_file_in_cp1251( name ):
	try:
		text = open( name, 'r', encoding='utf-8' ).read()
		text_bytes = text.encode( 'cp1251', errors='replace' )
		open( name + '.cp1251', 'wb' ).write( text_bytes )
	except UnicodeDecodeError:
		print( 'Error: ' + name )

def stem_file( name ):
	if path.isfile( name + '.json' ):
		return
	call([mystem_path, mystem_flags, "--eng-gr", "-e", "cp1251",
		"--format", "json", name, name + '.json' ])
			
def test_file( name ):
	name = name
	with open( name + '.crf-test', 'wb' ) as file:
		call([crf_test_path, "-m", crf_test_model_path, name ], stdout=file )

for ( dirpath, dirnames, filenames ) in walk( texts_path ):
	for filename in filenames:
		if filename.endswith( '.txt' ):
			print( filename )
			name = texts_path + filename
			save_file_in_cp1251( name )
			name += '.cp1251'
			stem_file( name )
			call_main_program( ['--prepare-test-file', name + '.json'], \
				name + '.json.signs' )
			test_file( name + '.json.signs' )
			call_main_program( ['--prepare-answer-file', name + '.json', \
				name + '.json.signs.crf-test'], name + '.answer' )
