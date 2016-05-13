#!/usr/local/bin/python3

__author__ = "Anton Todua"

from os import walk
from os import path
from subprocess import call

texts_path = "../test-texts/"
mystem_path = "../mystem"
mystem_flags = "-ncisd"
main_program_path = "../NamedEntityRecognition"
main_program_tokens_arg = "--tokens"
crf_test_path = "../CRF++-0.58/crf_test" 
crf_test_model_path = "../model.crf-model"

def save_file_in_cp1251( name ):
	try:
		text = open( name, 'r', encoding='utf-8' ).read()
		text_bytes = text.encode( 'cp1251', errors='replace' )
		open( name + '.cp1251', 'wb' ).write( text_bytes )
	except UnicodeDecodeError:
		print( 'Error: ' + name )

def save_answer( name ):
	text = open( name, 'r', encoding='cp1251' ).read()
	signs = open( name + '.json.signs.crf-test', 'r', encoding='cp1251' ).readlines()
	competiton_signs = open( name + '.json.signs.tokens', 'r', encoding='cp1251' ).readlines()
	answer_file = open( name + '.answer', 'w' )
	if( ( len( competiton_signs ) - len( signs ) ) > 1 ):
		return
	for i in range( 0,  min( len( competiton_signs ), len( signs ) ) ):
		(offset, length, token1) = competiton_signs[i].split()
		line_signs = signs[i].split()
		answer = line_signs[-1].upper()[0:3]
		if answer == "NO":
			continue
		original_token = text[int(offset):int(offset) + int(length)]
		print( answer + '\t' + offset + '\t'
			+ length + '\t# ' + original_token, file=answer_file )
		assert( token1 == line_signs[0] )
		assert( token1 == original_token )
		#print( answer + '\t' + offset + '\t' + length, file=answer_file )

def stem_file( name ):
	if path.isfile( name + '.json' ):
		return
	call([mystem_path, mystem_flags, "--eng-gr", "-e", "cp1251",
		"--format", "json", name, name + '.json' ])
		
def signs_file( name, tokens_only = False ):
	tokens_suffix = ''
	tokens_arg = []
	if tokens_only:
		tokens_suffix = '.tokens'
		tokens_arg = [main_program_tokens_arg]
	with open( name + '.signs' + tokens_suffix, 'wb' ) as file:
		call( [main_program_path, name] + tokens_arg, stdout=file )
			
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
			signs_file( name + '.json' )
			test_file( name + '.json.signs' )
			signs_file( name + '.json', True )
			save_answer( name )

