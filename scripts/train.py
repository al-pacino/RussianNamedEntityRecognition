#!/usr/local/bin/python3

__author__ = "Anton Todua"

from os import walk
from os import path
from subprocess import call

texts_path = '../train-texts/'
target_train_file = '../train-texts.cp1251.signs'
mystem_path = "../mystem"
mystem_flags = "-ncisd"
main_program_path = "../NamedEntityRecognition"
train_file_line_before_signs_file = \
	'begin-of-file	begin-of-file	NO	NO	L1	begin-of-file	NO	NO	NO	NO	NO	NO	NO	YES	NO	R0	NO	NO'
train_file_line_after_signs_file = \
	'end-of-file	end-of-file	NO	NO	L1	end-of-file	NO	NO	NO	NO	NO	NO	NO	YES	NO	R0	NO	NO'

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

def save_tokens_to_ann( filename, ann_filename ):
	with open( ann_filename + '.ann', 'w' ) as file:
		for span in open( filename, 'r', encoding='utf-8' ):
			if not span.strip():
				continue
			( ner_type, offset, length ) = span.split()[1:4]
			if ner_type == 'name' \
					or ner_type == 'surname' \
					or ner_type == 'nickname' \
					or ner_type == 'patronymic':
				ner_type = 'PER'
			elif ner_type == 'loc_name' \
					or ner_type == 'loc_descr' \
					or ner_type == 'geo_adj':
				ner_type = 'LOC'
			elif ner_type == 'org_name' \
					or ner_type == 'org_descr':
				ner_type = 'ORG'
			elif ner_type == 'job' \
					or ner_type == 'prj_name' \
					or ner_type == 'prj_descr':
				continue
			else:
				print( filename + ':' + ner_type )
				assert( False )
			print( ner_type + '\t' + offset + '\t' \
				+ str( int( offset ) + int( length ) ), file=file )
		
with open( target_train_file, 'w' ) as file:
	for ( dirpath, dirnames, filenames ) in walk( texts_path ):
		for filename in filenames:
			if filename.endswith( '.txt' ):
				print( filename )
				name = texts_path + filename
				save_file_in_cp1251( name )
				name += '.cp1251'
				stem_file( name )
				name += '.json'
				save_tokens_to_ann( texts_path + filename[:-4] + '.spans', name )
				call_main_program( ['--prepare_train_file', name, \
					name + '.ann'],	name + '.signs' )
				# save all .sings to target_train_file
				print( train_file_line_before_signs_file, file=file )
				print( train_file_line_before_signs_file, file=file )
				file.write( open( name + '.signs', 'r' ).read() )
				print( train_file_line_after_signs_file, file=file )
				print( train_file_line_after_signs_file, file=file )
