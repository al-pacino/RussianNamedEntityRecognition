#include <cassert>
#include <map>
#include <set>
#include <memory>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <unordered_set>
#include <unordered_map>

#ifdef _WIN32
#include <Windows.h>
#undef GetObject // conflicts with rapidjson
#endif

#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>

using namespace std;

//-----------------------------------------------------------------------------

const char* const AuxFileRelativePath =
	"lowercase-cp1251-aux-files/lowercase-cp1251-";

//-----------------------------------------------------------------------------

class CException {
public:
	CException( const string& _message = "" ) :
		message( _message )
	{
	}

	const string& Message() const { return message; }
	void Delete() { delete this; }

private:
	string message;
};

//-----------------------------------------------------------------------------

enum TTokenType {
	TT_None,
	TT_Text,
	TT_EngWord, // english
	TT_A, // прилагательное
	TT_ADV, // наречие
	TT_ADVPRO, // местоименное наречие
	TT_ANUM, // числительное-прилагательное
	TT_APRO, // местоимение-прилагательное
	TT_COM, // часть композита - сложного слова
	TT_CONJ, // союз
	TT_INTJ, // междометие
	TT_NUM, // числительное
	TT_PART, // частица
	TT_PR, // предлог
	TT_S, // существительное
	TT_SPRO, // местоимение-существительное
	TT_V, // глагол
	TT_Comma, // ,
	TT_Dot, // .
	TT_Question, // ?
	TT_Exclamation, // !
	TT_Quote, // ", ', ј, Ь
	TT_LeftParen, // (
	TT_RightParen, // )
	TT_Semicolon, // ;
	TT_Colon, // :
	TT_Dash // -, ?, Д
};

const char* TokenTypesText[] = {
	"None",
	"Text",
	"EngWord",
	"A",
	"ADV",
	"ADVPRO",
	"ANUM",
	"APRO",
	"COM",
	"CONJ",
	"INTJ",
	"NUM",
	"PART",
	"PR",
	"S",
	"SPRO",
	"V",
	"Comma",
	"Dot",
	"Question",
	"Exclamation",
	"Quote",
	"LeftParen",
	"RightParen",
	"Semicolon",
	"Colon",
	"Dash"
};

bool IsWord( TTokenType tokenType )
{
	return ( tokenType >= TT_EngWord && tokenType <= TT_V );
}

bool IsMark( TTokenType tokenType )
{
	return ( tokenType >= TT_Comma && tokenType <= TT_Dash );
}

bool IsWordOrMark( TTokenType tokenType )
{
	return IsWord( tokenType ) || IsMark( tokenType );
}

//-----------------------------------------------------------------------------

struct CTextToTokenType {
	const char* Text;
	const TTokenType Type;
};

CTextToTokenType TextToPartOfSpeech[] = {
	{ "A", TT_A },
	{ "ADV", TT_ADV },
	{ "ADVPRO", TT_ADVPRO },
	{ "ANUM", TT_ANUM },
	{ "APRO", TT_APRO },
	{ "COM", TT_COM },
	{ "CONJ", TT_CONJ },
	{ "INTJ", TT_INTJ },
	{ "NUM", TT_NUM },
	{ "PART", TT_PART },
	{ "PR", TT_PR },
	{ "S", TT_S },
	{ "SPRO", TT_SPRO },
	{ "V", TT_V },
	{ 0, TT_None }
};

// returns the part of spich by mystem 'gr'
TTokenType GetTypeByGr( const string& gr )
{
	static unordered_map<string, TTokenType> textToPartOfSpeech;
	if( textToPartOfSpeech.empty() ) {
		for( int i = 0; TextToPartOfSpeech[i].Text != 0; i++ ) {
			textToPartOfSpeech.insert( make_pair(
				TextToPartOfSpeech[i].Text, TextToPartOfSpeech[i].Type ) );
		}
	}
	size_t pos = gr.find_first_of( ",=" );
	if( pos != string::npos ) {
		auto i = textToPartOfSpeech.find( gr.substr( 0, pos ) );
		if( i != textToPartOfSpeech.end() ) {
			return i->second;
		}
	}
	return TT_None;
}

//-----------------------------------------------------------------------------

CTextToTokenType PunctuationsToPartOfSpeech[] = {
	{ ",", TT_Comma },
	{ ".", TT_Dot },
	{ "?", TT_Question },
	{ "!", TT_Exclamation },
	{ "'\"Ђї", TT_Quote },
	{ "(", TT_LeftParen },
	{ ")", TT_RightParen },
	{ ";", TT_Semicolon },
	{ ":", TT_Colon },
	{ "-ЦЧ", TT_Dash },
	{ 0, TT_None }
};

char MarkToLex( TTokenType tokenType )
{
	assert( IsMark( tokenType ) );
	return PunctuationsToPartOfSpeech[tokenType - TT_Comma].Text[0];
}

TTokenType GetTypeByPunctuation( char punctuation )
{
	static unordered_map<char, TTokenType> punctuationsToPartOfSpeech;
	if( punctuationsToPartOfSpeech.empty() ) {
		for( int i = 0; PunctuationsToPartOfSpeech[i].Text != 0; i++ ) {
			for( const char* c = PunctuationsToPartOfSpeech[i].Text;
				*c != '\0'; ++c )
			{
				punctuationsToPartOfSpeech.insert( make_pair( *c,
					PunctuationsToPartOfSpeech[i].Type ) );
			}
		}
	}
	auto i = punctuationsToPartOfSpeech.find( punctuation );
	if( i != punctuationsToPartOfSpeech.end() ) {
		return i->second;
	}
	return TT_None;
}

//-----------------------------------------------------------------------------

enum TNamedEntityType {
	NET_None,
	NET_Org,
	NET_Loc,
	NET_Person
};

const char* const NamedEntityTypesText[] = {
	"NO",
	"Org",
	"Loc",
	"Person"
};

struct CToken {
	TTokenType Type;
	string Lex;
	string Text;
	bool IsEndOfSentence;
	TNamedEntityType NamedEntityType;

	CToken( TTokenType tokenType = TT_None,
			const string& lex = "", const string& text = "" ) :
		Type( tokenType ),
		Lex( lex ),
		Text( text ),
		IsEndOfSentence( false ),
		NamedEntityType( NET_None )
	{
	}

	void Print() const;
};

void CToken::Print() const
{
	cout << "{" << TokenTypesText[Type] << "}"
		<< "{" << Text << "}" << "{" << Lex << "}"
		<< ( IsEndOfSentence ? " end of sentence." : "" )
		<< endl;
}

//-----------------------------------------------------------------------------

class CTokens : public vector<CToken> {
public:
	void AddWord( const char* text, const char* lex, const char* gr );
	void AddPunctuationMarks( const char* text );
};

void CTokens::AddWord( const char* text, const char* lex, const char* gr )
{
	TTokenType tokenType = GetTypeByGr( gr );
	if( tokenType != TT_None ) {
		push_back( CToken( tokenType, lex, text ) );
	}
}

void CTokens::AddPunctuationMarks( const char* text )
{
	if( string( text ) == "\\s" ) { // end of sentence
		auto ri = rbegin();
		while( ri != rend() && !IsWordOrMark( ri->Type ) ) {
			++ri;
		}
		if( ri != rend() ) {
			ri->IsEndOfSentence = true;
		}
		return;
	}

	if( empty() || back().Type != TT_Text ) {
		push_back( CToken( TT_Text ) );
	}
	back().Text += text;

	for( const char* c = text; *c != '\0'; c++ ) {
		if( *c == ' ' ) {
			continue;
		}
		TTokenType tokenType = GetTypeByPunctuation( *c );
		if( tokenType == TT_None ) {
			continue;
		}
		push_back( CToken( tokenType,
			string( 1, MarkToLex( tokenType ) ), string( 1, *c ) ) );
	}
}

//-----------------------------------------------------------------------------
// CBaseSign

class CBaseSign {
public:
	CBaseSign() {}

	static const char* const BinarySignTrue;
	static const char* const BinarySignFalse;

	virtual string Value( const CToken& token ) const = 0;

private:
	CBaseSign( const CBaseSign& );
	CBaseSign& operator=( const CBaseSign& );
};

const char* const CBaseSign::BinarySignTrue = "YES";
const char* const CBaseSign::BinarySignFalse = "NO";

//-----------------------------------------------------------------------------
// CSigns

class CSigns : public vector<shared_ptr<CBaseSign> > {
public:
	void AddSign( CBaseSign* sign );
	void Apply( const CToken& token, string& result,
		const string& signsSep = "\t" ) const;
};

void CSigns::AddSign( CBaseSign* sign )
{
	push_back( shared_ptr<CBaseSign>( sign ) );
}

void CSigns::Apply( const CToken& token, string& result,
	const string& signsSep ) const
{
	result.clear();
	for( auto i = cbegin(); i != cend(); ++i ) {
		string signValue = ( *i )->Value( token );
		assert( !signValue.empty() );
		if( !result.empty() ) {
			result += signsSep;
		}
		result += signValue;
	}
}

//-----------------------------------------------------------------------------
// CTextSign

class CTextSign : public CBaseSign {
public:
	virtual string Value( const CToken& token ) const;
};

string CTextSign::Value( const CToken& token ) const
{
	assert( IsWordOrMark( token.Type ) );
	return token.Text;
}

//-----------------------------------------------------------------------------
// CLexSign

class CLexSign : public CBaseSign {
public:
	virtual string Value( const CToken& token ) const;
};

string CLexSign::Value( const CToken& token ) const
{
	assert( IsWordOrMark( token.Type ) );
	return token.Lex;
}

//-----------------------------------------------------------------------------
// CNamedEntityTypeSign

class CNamedEntityTypeSign : public CBaseSign {
public:
	virtual string Value( const CToken& token ) const;
};

string CNamedEntityTypeSign::Value( const CToken& token ) const
{
	assert( IsWordOrMark( token.Type ) );
	return NamedEntityTypesText[token.NamedEntityType];
}

//-----------------------------------------------------------------------------
// CLetterSign

class CLetterSign : public CBaseSign {
public:
	CLetterSign( const char* letters );

	bool IsLetterSatisfy( char letter ) const;

private:
	vector<bool> mask;
};

CLetterSign::CLetterSign( const char* letters )
{
	const unsigned char Size = ~0;
	mask.resize( Size + 1 );
	for( int i = 0; letters[i] != '\0'; i++ ) {
		vector<bool>::size_type f = static_cast<unsigned char>( letters[i] );
		mask[f] = true;
	}
}

bool CLetterSign::IsLetterSatisfy( char letter ) const
{
	return mask[static_cast<unsigned char>( letter )];
}

//-----------------------------------------------------------------------------
// CRegisterSign

class CRegisterSign : public CLetterSign {
public:
	CRegisterSign() :
		CLetterSign( UppercaseLetters )
	{
	}

	static const char* const UppercaseLetters;

	virtual string Value( const CToken& token ) const;
};

const char* const CRegisterSign::UppercaseLetters =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	"јЅ¬√ƒ≈®∆«»… ЋћЌќѕ–—“”‘’÷„ЎўЏџ№Ёёя";

string CRegisterSign::Value( const CToken& token ) const
{
	assert( IsWordOrMark( token.Type ) );
	if( IsWord( token.Type ) ) {
		assert( !token.Text.empty() );
		bool allInUppercase = true;
		bool allInLowercase = true;
		for( string::size_type i = 1; i < token.Text.length(); i++ ) {
			const bool isLetterInUppercase = IsLetterSatisfy( token.Text[i] );
			allInUppercase &= isLetterInUppercase;
			allInLowercase &= !isLetterInUppercase;
		}
		const bool isFirstLetterInUppercase = IsLetterSatisfy( token.Text[0] );
		if( isFirstLetterInUppercase && allInUppercase ) {
			return "BigBig";
		} else if( isFirstLetterInUppercase && allInLowercase ) {
			return "BigSmall";
		} else if( !isFirstLetterInUppercase && allInLowercase ) {
			return "Small";
		}
		return "Fence";
	}
	return BinarySignFalse;
}

//-----------------------------------------------------------------------------
// CHasVowelLetterSign

class CHasVowelLetterSign : public CLetterSign {
public:
	CHasVowelLetterSign() :
		CLetterSign( WovelLetters )
	{
	}

	static const char* const WovelLetters;

	virtual string Value( const CToken& token ) const;
};

const char* const CHasVowelLetterSign::WovelLetters =
	"ауоыиэ€юЄе" "aeiou"
	"ј”ќџ»Ёяё®≈" "AEIOU";

string CHasVowelLetterSign::Value( const CToken& token ) const
{
	assert( IsWordOrMark( token.Type ) );
	if( IsWord( token.Type ) ) {
		assert( !token.Text.empty() );
		for( string::size_type i = 0; i < token.Text.length(); i++ ) {
			if( IsLetterSatisfy( token.Text[i] ) ) {
				return BinarySignTrue;
			}
		}
	}
	return BinarySignFalse;
}

//-----------------------------------------------------------------------------
// CTextLengthSign

class CTextLengthSign : public CBaseSign {
public:
	virtual string Value( const CToken& token ) const;
};

string CTextLengthSign::Value( const CToken& token ) const
{
	assert( IsWordOrMark( token.Type ) );
	if( IsMark( token.Type ) || token.Text.length() == 1 ) {
		return "L1";
	} else if( token.Text.length() >= 5 ) {
		return "L5+";
	} else {
		return "L2-4";
	}
}

//-----------------------------------------------------------------------------
// CTokenTypeSign

class CTokenTypeSign : public CBaseSign {
public:
	virtual string Value( const CToken& token ) const;
};

string CTokenTypeSign::Value( const CToken& token ) const
{
	assert( IsWordOrMark( token.Type ) );
	return TokenTypesText[token.Type];
}

//-----------------------------------------------------------------------------
// CIsEndOfSentenceSign

class CIsEndOfSentenceSign : public CBaseSign {
public:
	virtual string Value( const CToken& token ) const;
};

string CIsEndOfSentenceSign::Value( const CToken& token ) const
{
	assert( IsWordOrMark( token.Type ) );
	if( token.IsEndOfSentence ) {
		return BinarySignTrue;
	}
	return BinarySignFalse;
}

//-----------------------------------------------------------------------------
// CFileSign

class CFileSign : public CBaseSign {
public:
	CFileSign( const string& fileName );

protected:
	unordered_set<string> words;
};

CFileSign::CFileSign( const string& fileName )
{
	ifstream file( fileName );
	while( file.good() ) {
		string word;
		getline( file, word );
		if( !word.empty() ) {
			words.insert( word );
		}
	}
	if( words.empty() ) {
		throw new CException( "File '" + fileName + "' is empty!" );
	}
}

//-----------------------------------------------------------------------------
// CLexFromFileSign

class CLexFromFileSign : public CFileSign {
public:
	CLexFromFileSign( const string& fileName ) :
		CFileSign( fileName )
	{
	}

	virtual string Value( const CToken& token ) const;
};

string CLexFromFileSign::Value( const CToken& token ) const
{
	assert( IsWordOrMark( token.Type ) );
	if( IsWord( token.Type ) ) {
		auto i = words.find( token.Lex );
		if( i != words.end() ) {
			return BinarySignTrue;
		}		
	}
	return BinarySignFalse;
}

//-----------------------------------------------------------------------------
// CHasPrefixFromFileSign

class CHasPrefixFromFileSign : public CFileSign {
public:
	CHasPrefixFromFileSign( const string& fileName ) :
		CFileSign( fileName )
	{
	}

	virtual string Value( const CToken& token ) const;
};

string CHasPrefixFromFileSign::Value( const CToken& token ) const
{
	return BinarySignFalse;
}

//-----------------------------------------------------------------------------
// CHasSuffixFromFileSign

class CHasSuffixFromFileSign : public CFileSign {
public:
	CHasSuffixFromFileSign( const string& fileName ) :
		CFileSign( fileName )
	{
	}

	virtual string Value( const CToken& token ) const;
};

string CHasSuffixFromFileSign::Value( const CToken& token ) const
{
	for( auto i = words.cbegin(); i != words.cend(); ++i ) {
		const string& lex = token.Lex;
		const string::size_type ll = lex.length(); // lex length
		const string::size_type sl = i->length(); // suffix length
		if( ll >= sl && lex.compare( ll - sl, sl, *i ) ) {
			return BinarySignTrue;
		}
	}
	return BinarySignFalse;
}

//-----------------------------------------------------------------------------
// CHasRootFromFileSign

class CHasRootFromFileSign : public CFileSign {
public:
	CHasRootFromFileSign( const string& fileName ) :
		CFileSign( fileName )
	{
	}

	virtual string Value( const CToken& token ) const;
};

string CHasRootFromFileSign::Value( const CToken& token ) const
{
	int numberOfRootOccurencies = 0;
	for( auto i = words.cbegin(); i != words.cend(); ++i ) {
		if( token.Lex.find( *i ) != string::npos ) {
			numberOfRootOccurencies++;
			if( numberOfRootOccurencies >= 2 ) {
				break;
			}
		}
	}
	assert( numberOfRootOccurencies <= 2 );
	if( numberOfRootOccurencies == 1 ) {
		return "R1";
	} else if( numberOfRootOccurencies == 2 ) {
		return "R2+";
	}
	return "R0";
}

//-----------------------------------------------------------------------------

void InitializeSigns( CSigns& signs, const string& auxFilesPath )
{
	signs.clear();

	// “окен (форма из текста)
	signs.AddSign( new CTextSign() );

	// Ћемма (начальна€ форма из mystem)
	signs.AddSign( new CLexSign() );

	// –егистр (BigBig/BigSmall/SmallSmall/Fence)
	signs.AddSign( new CRegisterSign() );

	// Ѕинарный признак наличие гласной
	signs.AddSign( new CHasVowelLetterSign() );

	// ƒиапазон длинны слова от оригинала(токена) (1/2-4/5-more)
	signs.AddSign( new CTextLengthSign() );

	// „асть речи (дл€ слов Ч части речи, дл€ пунктуации Ч вид знака
	signs.AddSign( new CTokenTypeSign() );

	//  ончаетс€ ли слово известным окончанием фамилии (файл)
	signs.AddSign( new CHasSuffixFromFileSign( auxFilesPath + "surname_endings.txt" ) );

	// явл€етс€ ли именем человека (файл)
	signs.AddSign( new CLexFromFileSign( auxFilesPath + "first_names.txt" ) );

	// —писок слов предшествующих организаци€м (файл)
	signs.AddSign( new CLexFromFileSign( auxFilesPath + "before_organizations.txt" ) );

	// —писок слов предшествующих локаци€м (файл)
	signs.AddSign( new CLexFromFileSign( auxFilesPath + "before_location_words.txt" ) );

	// —писок столиц (файл)
	signs.AddSign( new CLexFromFileSign( auxFilesPath + "capitals.txt" ) );

	// —писок государств (файл)
	signs.AddSign( new CLexFromFileSign( auxFilesPath + "states.txt" ) );

	// —писок фамилий (файл)
	signs.AddSign( new CLexFromFileSign( auxFilesPath + "surnames.txt" ) );

	// явл€етс€ ли токен концом предложени€? (бинарный) (точка или последнее слово)
	signs.AddSign( new CIsEndOfSentenceSign );

	// Ќазвание валюты (файл)
	signs.AddSign( new CLexFromFileSign( auxFilesPath + "monetary_units.txt" ) );

	// —писок корней организаций (файлик) -> диапазон количества корней в слове (0/1/2-more)
	signs.AddSign( new CHasRootFromFileSign( auxFilesPath + "company_parts_for_count.txt" ) );

	// —писок начал имен (файл)
	// signs.AddSign( new CHasPrefixFromFileSign( auxFilesPath + "first_name_prefixes.txt" ) );

	// Cписок отчеств (файл)
	signs.AddSign( new CLexFromFileSign( auxFilesPath + "patronymics.txt" ) );

	// ¬ид именованной сущности
	signs.AddSign( new CNamedEntityTypeSign() );
}

void ReadTokens( const string& textFileName, CTokens& tokens )
{
	tokens.clear();
	ifstream inputFile( textFileName );
	while( inputFile.good() ) {
		string line;
		getline( inputFile, line );
		if( line.empty() ) {
			continue;
		}
		rapidjson::Document document;
		document.Parse( line.c_str(), line.length() );
		if( !document.IsObject() ) {
			throw new CException( "Can't read json object from line '"
				+ line + "' of file '" + textFileName + "'" );
		}
		rapidjson::Value token = document.GetObject();
		const char* text = token["text"].GetString();
		if( token.HasMember( "analysis" ) ) {
			rapidjson::Value lexArray = token["analysis"].GetArray();
			if( lexArray.Size() > 0 ) {
				rapidjson::Value firstLex = lexArray[0].GetObject();
				tokens.AddWord( text, firstLex["lex"].GetString(),
					firstLex["gr"].GetString() );
			} else {
				tokens.push_back( CToken( TT_EngWord, text, text ) );
			}
		} else {
			tokens.AddPunctuationMarks( text );
		}
	}
	if( tokens.empty() ) {
		throw new CException( "There are no tokens in file '"
			+ textFileName + "'" );
	}
}

void AddTokenToOffset( const CToken& token, size_t& offset )
{
	assert( token.Type != TT_None );
	if( !IsMark( token.Type ) ) {
#ifdef _DEBUG
		cout << offset << " " << token.Text << endl;
#endif
		offset += token.Text.length();
	}
}

void ReadAnswer( const string& answerFileName, CTokens& tokens )
{
	map<size_t, pair<size_t, TNamedEntityType> > answers;
	// parse answer file
	ifstream inputFile( answerFileName );
	while( inputFile.good() ) {
		string line;
		getline( inputFile, line );
		if( line.empty() ) {
			continue;
		}
		istringstream iss( line );
		string type;
		size_t start;
		size_t end;
		iss >> type >> start >> end;
		TNamedEntityType namedEntityType = NET_None;
		if( type == "LOC" ) {
			namedEntityType = NET_Loc;
		} else if( type == "ORG" ) {
			namedEntityType = NET_Org;
		} else if( type == "PER" ) {
			namedEntityType = NET_Person;
		}
		if( iss.fail() || namedEntityType == NET_None || end <= start ) {
			throw new CException( "Bad answer file '"
				+ answerFileName + "' format" );
		}
		answers.insert( make_pair( start, make_pair( end, namedEntityType ) ) );
	}
	// set named entiry type for tokens
	size_t offset = 0;
	auto ti = tokens.begin();
	for( auto i = answers.cbegin(); i != answers.cend(); ++i ) {
		for( ; offset < i->first; ++ti ) {
			AddTokenToOffset( *ti, offset );
		}
		do {
			AddTokenToOffset( *ti, offset );
			if( !IsMark( ti->Type ) ) {
				ti->NamedEntityType = i->second.second;
			}
			ti++;
		} while( offset < i->second.first );
	}
}

string GetPath( const string& filename )
{
	string::size_type i = filename.find_last_of( "\\/" );
	if( i != string::npos ) {
		return filename.substr( 0, i + 1 );
	}
	return "./";
}

//------------------------------------------------------------------------------

void PrepareSigns( const string& auxFilesPath, const CTokens& tokens )
{
	// intialize token signs
	CSigns signs;
	InitializeSigns( signs, auxFilesPath );

	for( auto i = tokens.cbegin(); i != tokens.cend(); ++i ) {
		assert( i->Type != TT_None );
		if( i->Type == TT_Text ) {
			continue;
		}
		string line;
		signs.Apply( *i, line );
		cout << line << endl;
	}
}

//------------------------------------------------------------------------------

void PrepareTestFile( const char* argv[] )
{
	CTokens tokens;
	ReadTokens( argv[2], tokens );
	PrepareSigns( GetPath( argv[0] ) + AuxFileRelativePath, tokens );
}

//------------------------------------------------------------------------------

void PrepareTrainFile( const char* argv[] )
{
	CTokens tokens;
	ReadTokens( argv[2], tokens );
	ReadAnswer( argv[3], tokens );
	PrepareSigns( GetPath( argv[0] ) + AuxFileRelativePath, tokens );
}

//------------------------------------------------------------------------------

void PrepareAnswerFile( const char* argv[] )
{
	CTokens tokens;
	ReadTokens( argv[2], tokens );
	size_t offset = 0;
	for( auto i = tokens.begin(); i != tokens.end(); ++i ) {
		if( IsWordOrMark( i->Type ) ) {
			cout << offset << "\t" << i->Text.length() << "\t"
				<< i->Text << endl;
		}
		AddTokenToOffset( *i, offset );
	}
}

//------------------------------------------------------------------------------

typedef void ( *StartupFunctionPtr )( const char* argv[] );

struct StartupMode {
	const char* FirstArgument;
	int NumberOfArguments;
	StartupFunctionPtr StartupFunction;
	const char* HelpString;
};

const StartupMode StartupModes[] = {
	{ "--prepare_test_file", 3, PrepareTestFile,
		"--prepare_test_file TEXT_JSON_FILE" },

	{ "--prepare_train_file", 4, PrepareTrainFile,
		"--prepare_train_file TEXT_JSON_FILE TEXT_ANN_FILE" },

	{ "--prepare_answer_file", 4, PrepareAnswerFile,
		"--prepare_answer_file TEXT_JSON_FILE CRF_TESTED_FILE" },

	{ nullptr, -1, nullptr, nullptr }
};

//------------------------------------------------------------------------------

void PrintUsage()
{
	cerr << "Usage: NamedEntityRecognition" << endl;
	cerr << endl;
	for( int i = 0; StartupModes[i].FirstArgument != nullptr; i++ ) {
		assert( StartupModes[i].HelpString != nullptr );
		if( i > 0 ) {
			cerr << "OR" << endl;
		}
		cerr << "  " << StartupModes[i].HelpString << endl;
	}
	cerr << endl;
}

//------------------------------------------------------------------------------

bool Run( int argc, const char* argv[] )
{
	int startupMode = -1;
	if( argc >= 2 ) {
		const string firstArgument( argv[1] );

		for( int i = 0; StartupModes[i].FirstArgument != nullptr; i++ ) {
			if( firstArgument == StartupModes[i].FirstArgument ) {
				if( argc == StartupModes[i].NumberOfArguments ) {
					startupMode = i;
				}
				break;
			}
		}
	}

	if( startupMode != -1 ) {
		StartupModes[startupMode].StartupFunction( argv );
		return true;
	}

	PrintUsage();
	return false;
}

//------------------------------------------------------------------------------

int main( int argc, const char* argv[] )
{
#if defined( WIN32 ) && defined( _DEBUG )
	system( "chcp 1251" );
#endif

	bool success = false;

	try {
		success = Run( argc, argv );
	} catch( CException* e ) {
		cerr << "Error: " << e->Message() << endl;
		e->Delete();
	} catch( exception& e ) {
		cerr << "Error: std::exception: " << e.what() << endl;
	} catch( ... ) {
		cerr << "Error: Unhandled exception" << endl;
	}

	return ( success ? 1 : 0 );
}

