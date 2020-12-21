/*
	New York Times style Crossword Puzzle Generator
		Rotational Symmetry

	Sizes
		Small 7x7
		Standard 15x15 -- Can't compute, search space is too large.
		Large 21x21
		Mega 25x25

	Grid
		Size (row, col)

	Word
		string
		length

	Point
		Position (row, col)
		Letter
		Blank
		Block

	Span
		Set of Points
		String

	Slot
		EmptySlot
		PartialSlot
		FullSlot

	Design
		struct Word
		struct Point
		struct Span
		struct Slot
		class Grid
		class Library
		class Engine

		Data Structure
			Word
			Point
			Span
			Slot
			Grid
			Library

		Algorithm
			Pattern Hash Table
			Recursive Backtraking
*/

// Standard Include
#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <cassert>


namespace cw
{
	// ----- CWE INTERFACE DECLARATION -----
	// Basic data structures used by crossword engine
	struct Word
	{
		Word() {}
		Word(std::string sWord) : s(sWord) {}
		int len() const { return s.length(); }

		std::string s;
	};


	struct Point
	{
		Point() {}
		Point(int r, int c) : nRow(r), nCol(c) {}

		int nRow = 0;
		int nCol = 0;

		friend std::ostream& operator << (std::ostream& os, const Point& p);
	};


	struct Span
	{
		Span(Point p, int l, bool v) : point(p), nLen(l), bVert(v) {}

		Point GetPoint(const int i) const;

		Point point;
		int nLen = 0;
		bool bVert = false;

		friend std::ostream& operator << (std::ostream& os, const Span& s);
	};


	struct Attribute
	{
		bool IsEmpty() const { return bHasBlanks && !bHasLetters; }
		bool IsPartial() const { return bHasBlanks && bHasLetters; }
		bool IsFull() const { return !bHasBlanks && bHasLetters; }

		bool bHasLetters = false;
		bool bHasBlanks = false;
	};


	struct Slot
	{
		Slot(const Span& s, const std::string& p) : span(s), sPattern(p) {}
		Span span;
		std::string sPattern;
		friend std::ostream& operator << (std::ostream& os, const Slot& s);
	};


	// Represents the crossword grid
	struct Grid
	{
	public:
		Grid();
		Grid(std::string name);

	public:
		bool LoadFromFile(std::string sFileName);
		void FillSpans();
		void PrintGrid() const;
		void PrintSpans() const;

	public:
		int rows() const { return vecLines.size(); }
		int cols() const { return !vecLines.empty() ? vecLines[0].size() : 0; }
		int MaxSize() const { return std::max(rows(), cols()); }
		void CheckSize() const;
		std::string GetString(const Span& span, Attribute& attr) const;
		void SetString(const Span& span, const std::string& sWord);
		bool IsInBounds(const Point& p) const;
		bool IsBlock(const Point& p) const;
		bool IsBlank(const Point& p) const;
		bool IsLetter(const Point& p) const;
		char GetChar(const Point& p) const;
		void SetChar(Point& p, char ch);
		bool Next(Point& p, bool bVert);
		bool NextStopAtWrap(Point& p, bool bVert);
		void FillSpans(bool bVert);

	public:
		std::string sID;
		std::vector<std::string> vecLines;
		std::vector<Span> vecSpans;
	};


	// Represents the master word list
	class Library
	{
	public:
		Library();

	public:
		bool ReadFromFile(std::string sFileName, int nMaxSize);
		void ComputeStats();
		void PrintStats() const;

	public:
		std::string GetWord(int nIndex) const;
		bool IsWord(std::string sWord) const;
		std::vector<std::shared_ptr<Word>>* FindWord(const std::string& sPattern);

	public:
		std::string ToUpper(std::string sWord);
		void DebugBuckets() const;
		void CreatePatternHash(const std::shared_ptr<Word> word);

	private:
		std::vector<std::shared_ptr<Word>> m_vecWords; // Master list 
		std::unordered_map<std::string, std::vector<std::shared_ptr<Word>>> m_umapWords; // Pattern Hash
		std::vector<int> m_vecCounts;
	};



	// Represents the duplicated word list
	struct StringSet
	{
		bool ExistsInSet(const std::unordered_set<std::string>& set, const std::string& sWord);
		void AddToSet(std::unordered_set<std::string>& set, const std::string& sWord);

		std::unordered_set<std::string> set;
	};



	// The Crossword Engine itself
	class Engine
	{
	public:
		Engine();

	public:
		void Init(std::string sGridPath, std::string sLibraryPath);
		void Search();

	private:
		void Loop(Grid grid, int nDepth);
		void CommitSlot(Grid& grid, const Slot& slot, int nDepth);

	private:
		Grid* m_grid;
		Library m_lib;
		StringSet m_setStr;
	};
}




namespace cw
{
	// ----- CWE IMPLEMENTATION -----

	// cw::POINT IMPLEMENTATION
	std::ostream& operator << (std::ostream& os, const Point& p)
	{
		os << "(" << p.nRow << "," << p.nCol << ")";
		return os;
	}

	Point Span::GetPoint(const int i) const
	{
		assert(i >= 0 && i < nLen);
		return bVert ? Point(point.nRow + i, point.nCol) : Point(point.nRow, point.nCol + i);
	}



	// cw::SPAN IMPLEMENTATION
	std::ostream& operator << (std::ostream& os, const Span& s)
	{
		os << "[" << s.point << " len=" << s.nLen << " vert=" << s.bVert << "]";
		return os;
	}



	// cw::SLOT IMPLEMENTATION
	std::ostream& operator << (std::ostream& os, const Slot& s)
	{
		os << s.span << "'" << s.sPattern << "'";
		return os;
	}



	// cw::GRID IMPLEMENTATION
	Grid::Grid() {}

	Grid::Grid(std::string name) : sID(name) {}

	bool Grid::LoadFromFile(std::string sFileName)
	{
		std::ifstream f;
		f.open(sFileName);
		while (!f.eof())
		{
			std::string line;
			std::getline(f, line);
			//std::cout << line << " (" << line.size() << ")\n";

			if (!line.empty() && line[0] != '/')
				vecLines.push_back(line);
		}

		return true;
	}

	void Grid::PrintGrid() const
	{
		std::cout << "Grid ID: " << sID << " "
				  << "(rows=" << rows() << ", "
				  << "cols=" << cols() << ")"
				  << " max_size=" << MaxSize() << "\n";

		for (auto& s : vecLines)
			std::cout << "  " << s << "\n";
	}

	void Grid::PrintSpans() const
	{
		std::cout << "Spans:" << "\n";
		for (const auto& span : vecSpans)
		{
			Attribute attr;
			std::cout << "  " << span << " " << GetString(span, attr) << "\n";
		}
	}

	// Add to 'spans' vector with all viable spans in the grid
	void Grid::FillSpans()
	{
		assert(vecSpans.empty());
		FillSpans(false);  // Horizontal Walk
		FillSpans(true);   // Vertical Walk
	}

	void Grid::CheckSize() const
	{
		for (auto s : vecLines)
			assert(s.size() == cols());
	}

	// Fills in attributes of the string
	std::string Grid::GetString(const Span& span, Attribute& attr) const
	{
		int len = span.nLen;
		std::string s;
		s.resize(len);
		for (int i = 0; i < len; i++)
		{
			Point p = span.GetPoint(i);
			char c = GetChar(p);
			if (c == '.')
				attr.bHasBlanks = true;
			else if (c >= 'A' && c <= 'Z')
				attr.bHasLetters = true;

			s[i] = c;
		}

		return s;
	}

	void Grid::SetString(const Span& span, const std::string& str)
	{
		int len = span.nLen;
		assert(len == str.length());
		for (int i = 0; i < len; i++)
		{
			Point p = span.GetPoint(i);
			SetChar(p, str[i]);
		}
	}

	bool Grid::IsInBounds(const Point& p) const
	{
		return (p.nRow >= 0 && p.nRow < rows() && p.nCol >= 0 && p.nCol < cols());
	}

	// Returns true if the point p is a '#' "block" in the grid, 'p' must be in bounds
	bool Grid::IsBlock(const Point& p) const
	{
		return GetChar(p) == '#';
	}

	// Returns true if the point p is a '.' "blank" in the grid, 'p' must be in bounds
	bool Grid::IsBlank(const Point& p) const
	{
		return GetChar(p) == '.';
	}
	// Returns true if the point p is a 'A'-'Z' "letter" in the grid, 'p' must be in bounds
	bool Grid::IsLetter(const Point& p) const
	{
		char c = GetChar(p);
		return (c >= 'A' && c <= 'Z');
	}

	char Grid::GetChar(const Point& p) const
	{
		assert(IsInBounds(p));
		return vecLines[p.nRow][p.nCol];
	}

	void Grid::SetChar(Point& p, char c)
	{
		assert(IsInBounds(p));
		vecLines[p.nRow][p.nCol] = c;
	}

	// Next increments the point across the grid, one box at a time.
	// Returns true if point is still in bounds
	bool Grid::Next(Point& p, bool vert)
	{
		if (vert)
		{
			p.nRow++;
			if (p.nRow >= rows())
			{
				p.nRow = 0;
				p.nCol++;
			}
		}
		else
		{
			p.nCol++;
			if (p.nCol >= cols())
			{
				p.nCol = 0;
				p.nRow++;
			}
		}

		return IsInBounds(p);
	}

	// NextStopAtWrap is like "Next" except it returns false at every wrap
	// Returns true if we stay on the same line
	bool Grid::NextStopAtWrap(Point& p, bool bVert)
	{
		bool bWrap = false;
		if (bVert)
		{
			p.nRow++;
			if (p.nRow >= rows())
			{
				p.nRow = 0;
				p.nCol++;
				bWrap = true;
			}
		}
		else
		{
			p.nCol++;
			if (p.nCol >= cols())
			{
				p.nCol = 0;
				p.nRow++;
				bWrap = true;
			}
		}

		return !bWrap;
	}

	// Main Routine
	void Grid::FillSpans(bool vert)
	{
		Point p;
		while (IsInBounds(p))
		{
			while (IsInBounds(p) && IsBlock(p))
				Next(p, vert);

			if (!IsInBounds(p))
				return;

			Point p0 = p;
			//cout << "SPAN START: " << p << "\n";

			// Count the length of the span
			int len = 0;
			bool keep_going = false;
			do
			{
				keep_going = NextStopAtWrap(p, vert);
				len++;
			} while (keep_going && !IsBlock(p));

			//cout << "SPAN END: len=" << len << "\n";
			vecSpans.push_back(Span(p0, len, vert));
		}
	}



	// cw::LIBRARY IMPLEMENTATION
	Library::Library() {}

	bool Library::ReadFromFile(std::string sFileName, int nMaxSize)
	{
		std::ifstream f;
		f.open(sFileName);
		while (!f.eof())
		{
			std::string line;
			std::getline(f, line);
			//std::cout << line << " (" << line.size() << ")\n";
			if (!line.empty())
			{
				line = ToUpper(line);

				// Remove undesired characters
				int len = line.length();
				if (line[len - 1] == '\r' || line[len - 1] == ' ' || line[len - 1] == '\t')
					line = line.substr(0, len - 1);

				if (line.length() <= nMaxSize)
				{
					std::shared_ptr<Word> w = std::make_shared<Word>(line);
					m_vecWords.push_back(w);
					CreatePatternHash(w);
				}

				//std::cout << "DEBUG: bucket_count=" << m_umapWords.bucket_count() << "\n";
				//std::cout << "DEBUG: load_factor=" << m_umapWords.load_factor() << "\n";
			}
		}

		std::cout << "Read " << m_vecWords.size()
			      << " words from '" << sFileName << "'\n";

		return true;
	}

	void Library::ComputeStats()
	{
		assert(m_vecCounts.empty());
		m_vecCounts.resize(18);
		for (const auto& w : m_vecWords)
		{
			int len = w->s.length();
			if (len < 18)
				m_vecCounts[len]++;
		}
	}

	void Library::PrintStats() const
	{
		std::cout << "Word Frequency Distribution" << "\n";
		for (int i = 1; i < m_vecCounts.size(); i++)
			std::cout << "[" << i << "] " << m_vecCounts[i] << "\n";
	}

	std::string Library::GetWord(int i) const
	{
		assert(i >= 0 && i < m_vecWords.size());
		return m_vecWords[i]->s;
	}

	bool Library::IsWord(std::string sWord) const
	{
		auto it = m_umapWords.find(sWord);
		return it != m_umapWords.end();
	}

	// Returns nullptr if can't find any matches to the given pattern
	std::vector<std::shared_ptr<Word>>* Library::FindWord(const std::string& sPattern)
	{
		auto it = m_umapWords.find(sPattern);
		if (it != m_umapWords.end())
			return &it->second;
		else
			return nullptr;
	}

	std::string Library::ToUpper(std::string sWord)
	{
		std::string s;
		for (char c : sWord)
			s.push_back(toupper(c));
		return s;
	}

	void Library::DebugBuckets() const
	{
		for (int i = 0; i < m_umapWords.bucket_count(); i++)
			std::cout << "[" << i << "] " << m_umapWords.bucket_size(i) << "\n";
	}

	// Pattern Hash Table Precomputation: this generates all possible word pattern permutations
	void Library::CreatePatternHash(const std::shared_ptr<Word> word)
	{
		int len = word->len();
		int nPatterns = pow(2, len); // 1 << len; (short hand for int 2^i in binary)
		//std::cout << "Pattern Hash on " << word->s << "\n";
		for (int i = 0; i < nPatterns; i++)
		{
			//std::cout << "  " << i << "\n";
			std::string s = word->s;
			for (int j = 0; j < len; j++)
				if ((i >> j) & 1) // Bit shift iteration to retrieve the bit mask
					s[j] = '.';

			//std::cout << s << "\n";
			m_umapWords[s].push_back(word);
		}
	}



	// cw::StringSet struct implementation
	bool StringSet::ExistsInSet(const std::unordered_set<std::string>& set, const std::string& s)
	{
		auto it = set.find(s);
		return it != set.end();
	}

	void StringSet::AddToSet(std::unordered_set<std::string>& set, const std::string& s)
	{
		assert(!ExistsInSet(set, s));
		set.insert(s);
	}



	// cw::Engine IMPLEMENTATION
	Engine::Engine() {}

	void Engine::Init(std::string sGridPath, std::string sLibraryPath)
	{
		// Load Grid
		m_grid = new cw::Grid("0");
		m_grid->LoadFromFile(sGridPath);
		m_grid->CheckSize();
		m_grid->FillSpans();
		m_grid->PrintSpans();

		// Load Library
		m_lib.ReadFromFile(sLibraryPath, m_grid->MaxSize());
	}

	void Engine::Search()
	{
		std::cout << "Searching grid" << "\n";
		m_grid->PrintGrid();
		Loop(*m_grid, 0);
	}

	void Engine::Loop(Grid grid, int nDepth)
	{
		std::chrono::system_clock::time_point t0 = std::chrono::system_clock::now();

		nDepth++;

		// Uncomment to control search tree depth
		/*if (nDepth > 5)
		{
			//std::cout << "Aborting recursion because depth=" << nDepth << "\n";
			return;
		}*/

		std::vector<Slot> vecEmptySlots;
		std::vector<Slot> vecPartialSlots; // These are the slots we want to search
		std::vector<Slot> vecFullSlots;

		for (const auto span : grid.vecSpans)
		{
			Attribute attr;
			std::string s = grid.GetString(span, attr); // pattern

			if (attr.IsEmpty())
				vecEmptySlots.push_back(Slot(span, s));
			else if (attr.IsPartial())
				vecPartialSlots.push_back(Slot(span, s));
			else if (attr.IsFull())
				vecFullSlots.push_back(Slot(span, s));
		}

		int nEmpty = vecEmptySlots.size();
		int nPartial = vecPartialSlots.size();
		int nFull = vecFullSlots.size();

		//std::cout << "empty=" << nEmpty << "\n";
		//std::cout << "partial=" << nPartial << "\n";
		//std::cout << "full=" << nFull << "\n";

		// Check if all commited words is a valid word
		for (const auto& w : vecFullSlots)
		{
			//cout << "Checking " << w.pattern << " if it is a word \n";
			if (!m_lib.IsWord(w.sPattern))
			{
				//std::cout << " --> Abort recursion\n";
				return;
			}
		}

		// Check if there are duplicates
		std::unordered_set<std::string> usetStrings;
		for (const auto& w : vecFullSlots)
		{
			if (m_setStr.ExistsInSet(usetStrings, w.sPattern))
			{
				//std::cout << " --> Abort recursion\n";
				return;
			}

			m_setStr.AddToSet(usetStrings, w.sPattern);
		}

		// Check the number of partials and empty
		if (nPartial == 0 && nEmpty == 0)
		{
			std::chrono::system_clock::time_point t1 = std::chrono::system_clock::now();

			// If there are not partial nor empty, we foud a solution
			std::cout << "Solution Found!" << "\n";
			grid.PrintGrid();
			std::cout << "\n";

			auto elapsedTime = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();
			std::cout << "Elapsed Time: " << elapsedTime << " ms" << "\n";

			// Add to Solution Grid vector?

			return;
		}

		assert(nPartial > 0);

		// Slot selection policy to reduce search space
		CommitSlot(grid, vecPartialSlots[0], nDepth);
	}

	void Engine::CommitSlot(Grid& grid, const Slot& slot, int nDepth)
	{
		//std::cout << "COMMIT slot" << slot << "\n";
		//std::cout << "Possible word choices for this slot are: \n";

		std::vector<std::shared_ptr<Word>>* words = m_lib.FindWord(slot.sPattern);

		if (words)
		{
			for (const auto w : *words)
			{
				assert(!words->empty());
				//std::cout << "Committing '" << w->s << "'\n";
				grid.SetString(slot.span, w->s);
				//std::cout << "New grid is\n";
				//grid.PrintGrid();
				Loop(grid, nDepth);
			}
		}
		else
		{
			//cout << "No mathes to pattern" << "\n";
		}
	}
}