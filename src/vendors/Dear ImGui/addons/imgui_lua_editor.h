#pragma once

#define NOMINMAX

#include <string>
#include <vector>
#include <array>
#include <memory>
#include <unordered_set>
#include <unordered_map>
#include <map>
#include <regex>
#include "..\imgui.h"

class TextEditor
{
public:
    enum class PaletteIndex
    {
        Default,
        PurpleKeyword,
        BlueKeyword,
        Number,
        String,
        CharLiteral,
        Punctuation,
        Identifier,
        KnownFunction,
        Comment,
        MultiLineComment,
        Background,
        Cursor,
        Selection,
        ErrorMarker,
        LineNumber,
        CurrentLineFill,
        CurrentLineFillInactive,
        CurrentLineEdge,
        Max
    };

    enum class SelectionMode
    {
        Normal,
        Word,
        Line
    };

    struct Coordinates
    {
        int mLine;
        int mColumn;
        Coordinates(int aLine = 0, int aColumn = 0) : mLine(aLine), mColumn(aColumn) {}

        bool operator ==(const Coordinates& o) const
        {
            return mLine == o.mLine && mColumn == o.mColumn;
        }

        bool operator !=(const Coordinates& o) const
        {
            return mLine != o.mLine || mColumn != o.mColumn;
        }

        bool operator <(const Coordinates& o) const
        {
            if (mLine != o.mLine)
                return mLine < o.mLine;
            return mColumn < o.mColumn;
        }

        bool operator >(const Coordinates& o) const
        {
            if (mLine != o.mLine)
                return mLine > o.mLine;
            return mColumn > o.mColumn;
        }

        bool operator <=(const Coordinates& o) const
        {
            if (mLine != o.mLine)
                return mLine < o.mLine;
            return mColumn <= o.mColumn;
        }

        bool operator >=(const Coordinates& o) const
        {
            if (mLine != o.mLine)
                return mLine > o.mLine;
            return mColumn >= o.mColumn;
        }
    };

    typedef std::map<int, std::string> ErrorMarkers;

    struct Glyph
    {
        char mChar;
        PaletteIndex mColorIndex = PaletteIndex::Default;
        bool mComment : 1;
        bool mMultiLineComment : 1;

        Glyph(char aChar, PaletteIndex aColorIndex) : mChar(aChar), mColorIndex(aColorIndex),
            mComment(false), mMultiLineComment(false) {}
    };

    struct LanguageDefinition
    {
        std::unordered_set<std::string> mPurpleKeywords;
        std::unordered_set<std::string> mBlueKeywords;
        std::unordered_set<std::string> mKnownFunctions;
        std::string mCommentStart, mCommentEnd, mSingleLineComment;
        char mPreprocChar;
        bool mAutoIndentation;

        bool(*mTokenize)(const char* in_begin, const char* in_end, const char*& out_begin, const char*& out_end, PaletteIndex& paletteIndex);

        std::vector<std::pair<std::string, PaletteIndex>> mTokenRegexStrings;

        bool mCaseSensitive;

        LanguageDefinition()
            : mPreprocChar('#'), mAutoIndentation(true), mTokenize(nullptr), mCaseSensitive(true)
        {
        }

        static const LanguageDefinition& CPlusPlus();
    };

    TextEditor();
    ~TextEditor();

    void SetErrorMarkers(const ErrorMarkers& aMarkers) { mErrorMarkers = aMarkers; }

    void Render(const char* aTitle, const ImVec2& aSize = ImVec2(), bool aBorder = false);
    void SetText(const std::string& aText);
    std::string GetText() const;
    std::string GetSelectedText() const;

    LanguageDefinition GetLanguageDefinition() { return mLanguageDefinition; }

    int GetTotalLines() const { return (int)mLines.size(); }

    void SetCursorPosition(const Coordinates& aPosition);

    void InsertText(const std::string& aValue);
    void InsertText(const char* aValue);

    void MoveUp(int aAmount = 1, bool aSelect = false);
    void MoveDown(int aAmount = 1, bool aSelect = false);
    void MoveLeft(int aAmount = 1, bool aSelect = false, bool aWordMode = false);
    void MoveRight(int aAmount = 1, bool aSelect = false, bool aWordMode = false);

    void SetSelection(const Coordinates& aStart, const Coordinates& aEnd, SelectionMode aMode = SelectionMode::Normal);
    bool HasSelection() const;

    void Copy();
    void Paste();

    void Undo(int aSteps = 1);
    void Redo(int aSteps = 1);


private:
    struct EditorState
    {
        Coordinates mSelectionStart;
        Coordinates mSelectionEnd;
        Coordinates mCursorPosition;
    };

    class UndoRecord
    {
    public:
        UndoRecord() {}
        ~UndoRecord() {}

        UndoRecord(
            const std::string& aAdded,
            const TextEditor::Coordinates aAddedStart,
            const TextEditor::Coordinates aAddedEnd,

            const std::string& aRemoved,
            const TextEditor::Coordinates aRemovedStart,
            const TextEditor::Coordinates aRemovedEnd,

            TextEditor::EditorState& aBefore,
            TextEditor::EditorState& aAfter);

        void Undo(TextEditor* aEditor);
        void Redo(TextEditor* aEditor);

        std::string mAdded;
        Coordinates mAddedStart;
        Coordinates mAddedEnd;

        std::string mRemoved;
        Coordinates mRemovedStart;
        Coordinates mRemovedEnd;

        EditorState mBefore;
        EditorState mAfter;
    };


    void Colorize(int aFromLine = 0, int aCount = -1);
    void ColorizeRange(int aFromLine = 0, int aToLine = 0);
    void ColorizeInternal();
    float TextDistanceToLineStart(const Coordinates& aFrom) const;
    std::string GetText(const Coordinates& aStart, const Coordinates& aEnd) const;
    Coordinates GetActualCursorCoordinates() const;
    Coordinates SanitizeCoordinates(const Coordinates& aValue) const;
    void Advance(Coordinates& aCoordinates) const;
    void DeleteRange(const Coordinates& aStart, const Coordinates& aEnd);
    int InsertTextAt(Coordinates& aWhere, const char* aValue);
    void AddUndo(UndoRecord& aValue);
    Coordinates ScreenPosToCoordinates(const ImVec2& aPosition) const;
    Coordinates FindWordStart(const Coordinates& aFrom) const;
    Coordinates FindWordEnd(const Coordinates& aFrom) const;
    Coordinates FindNextWord(const Coordinates& aFrom) const;
    int GetCharacterIndex(const Coordinates& aCoordinates) const;
    int GetCharacterColumn(int aLine, int aIndex) const;
    int GetLineCharacterCount(int aLine) const;
    int GetLineMaxColumn(int aLine) const;
    bool IsOnWordBoundary(const Coordinates& aAt) const;
    void RemoveLine(int aStart, int aEnd);
    void RemoveLine(int aIndex);
    std::vector<Glyph>& InsertLine(int aIndex);
    void EnterCharacter(ImWchar aChar, bool aShift);
    void Backspace();
    void DeleteSelection();
    std::string GetWordAt(const Coordinates& aCoords) const;
    ImColor GetGlyphColor(const Glyph& aGlyph) const;

    void HandleKeyboardInputs();
    void HandleMouseInputs();
    void Render();

    float mLineSpacing;
    std::vector<std::vector<Glyph>> mLines;
    EditorState mState;
    std::vector<UndoRecord> mUndoBuffer;
    int mUndoIndex;

    bool mReadOnly;
    bool mWithinRender;
    bool mScrollToTop;
    bool mTextChanged;
    float mTextStart;
    bool mCursorPositionChanged;
    int mColorRangeMin, mColorRangeMax;
    SelectionMode mSelectionMode;

    const std::vector<ImColor> mPalette =
    {
        ImColor(124, 220, 240, 255),	// Default
        ImColor(197, 134, 192, 255),	// Purple Keyword	
        ImColor(50, 122, 204, 255),	// Blue Keyword	
        ImColor(176, 182, 114, 255), // Number
        ImColor(214, 157, 133, 255), // String
        ImColor(214, 157, 133, 255), // Char literal
        0xffffffff, // Punctuation
        0xffaaaaaa, // Identifier
        ImColor(229, 235, 117, 255), // Known function
        ImColor(87, 162, 57, 255), // Comment (single line)
        ImColor(87, 162, 57, 255), // Comment (multi line)
        ImColor(30, 30, 30, 255), // Background
        0xffe0e0e0, // Cursor
        ImColor(58, 122, 186, 150), // Selection
        ImColor(221, 57, 50, 255), // ErrorMarker
        ImColor(43, 145, 175, 255), // Line number
        0x40000000, // Current line fill
        0x40808080, // Current line fill (inactive)
        0x40a0a0a0 // Current line edge
    };

    LanguageDefinition mLanguageDefinition;
    std::vector<std::pair<std::regex, PaletteIndex>> mRegexList;

    bool mCheckComments = true;
    ErrorMarkers mErrorMarkers;
    ImVec2 mCharAdvance;
    Coordinates mInteractiveStart, mInteractiveEnd;
    std::string mLineBuffer;
    uint64_t mStartTime;

    float mLastClick;
};