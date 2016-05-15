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

После запуска ./test.py в каталоге test долно появиться по 5 новых файлов для каждого файла вида xxx.txt, содержащего исходный текст:
      1. xxx.txt.cp1251 - Файл содержит текст исходного файла в кодировке cp1251
      2. xxx.txt.cp1251.json - Файл содержит результат обработки файла в кодировке cp1251 анализатором mystem
      3. xxx.txt.cp1251.json.signs - Файл содержит признаки выделенные для слов файла основной программой
      4. xxx.txt.cp1251.json.signs.crf-test - Файл содержит результат применения crf_test к xxx.txt.cp1251.json.signs
      5. xxx.txt.cp1251.answer - Файл содержит выделеннные именованные сущности из текста (в кодировке cp1251)

Файл ответа состоит из строк вида:
тип_именованной_сущности <пробел> смещение_отностельно_начала_текстового_файла <пробел> длина_именованной_сущности_в_байтах <пробел> #{текст_именованной_сущности}

Пример содержимого файла ответа:
ORG 42 14 #{Милли Меджлиса}
PER 44 15 #{Айдын Мирзазаде}
ORG 80 7 #{Росбалт}
LOC 93 6 #{Москве}
ORG 122 18 #{Нагорного Карабаха}
ORG 138 3 #{МИД}

В корне репозитория есть файл обученной модели model.crf-model
