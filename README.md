RussianNamedEntityRecognition
=============================

Installation
============

- Clone this repository
```sh
clone git@github.com:al-pacino/RussianNamedEntityRecognition.git
cd RussianNamedEntityRecognition
```

- Download mystem (https://tech.yandex.ru/mystem/#download) and put it to the RussianNamedEntityRecognition directory

- Build CRF++
```sh
tar xvf CRF++-0.58.tar
cd CRF++-0.58
./configure && make
```

- Build NamedEntityRecognition program (WINDOWS: Visual Studio project available in folder vs2010)
```sh
/build.sh
```

Running
=======

```bash
cd script
unzip test-texts.zip
./test.py
```

Remarks
=======

A trained model file (model.crf-model) exists in root of the repository.

After running you should have 5 new files for all source text files in the test-texts directory:

1. *.cp1251 - the file contains source text in cp1251 encoding
2. *.json - the file contains the result of processing source text in cp1251 encoding by mystem analyzer
3. *.signs - the file contains signs which have been extracted by main program
4. *.crf-tested - the file contains the result of application crf_test to *.txt.cp1251.json.signs
5. *.task1 - the file contains named entities which have been extracted by main program

Each line of an *.task1 file has the structure:
TYPE <one-space> OFFSET <one-space> LENGTH

Where:
- TYPE is one of three named entity types: ORG (organization), LOC (location), PER (person);
- OFFSET is offset in bytes from the beginning of the file;
- LENGTH is length of the text of the named entity;

