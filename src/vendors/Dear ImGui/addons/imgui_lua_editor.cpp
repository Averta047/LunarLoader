#include <algorithm>
#include <chrono>
#include <string>
#include <regex>
#include <cmath>

#include "imgui_lua_editor.h"

#define IMGUI_DEFINE_MATH_OPERATORS

template<class InputIt1, class InputIt2, class BinaryPredicate>
bool equals(InputIt1 first1, InputIt1 last1,
    InputIt2 first2, InputIt2 last2, BinaryPredicate p)
{
    for (; first1 != last1 && first2 != last2; ++first1, ++first2)
    {
        if (!p(*first1, *first2))
            return false;
    }
    return first1 == last1 && first2 == last2;
}

TextEditor::TextEditor()
    : mLineSpacing(1.0f)
    , mUndoIndex(0)
    , mWithinRender(false)
    , mScrollToTop(false)
    , mTextChanged(false)
    , mTextStart(20.0f)
    , mCursorPositionChanged(false)
    , mColorRangeMin(0)
    , mColorRangeMax(0)
    , mSelectionMode(SelectionMode::Normal)
    , mCheckComments(true)
    , mLastClick(-1.0f)
    , mStartTime(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count())
{

    mLanguageDefinition = LanguageDefinition::CPlusPlus();
    mRegexList.clear();

    for (auto& r : mLanguageDefinition.mTokenRegexStrings)
        mRegexList.push_back(std::make_pair(std::regex(r.first, std::regex_constants::optimize), r.second));

    Colorize();

    mLines.push_back(std::vector<TextEditor::Glyph>());
}

TextEditor::~TextEditor()
{
}

std::string TextEditor::GetText(const Coordinates& aStart, const Coordinates& aEnd) const
{
    std::string result;

    auto lstart = aStart.mLine;
    auto lend = aEnd.mLine;
    auto istart = GetCharacterIndex(aStart);
    auto iend = GetCharacterIndex(aEnd);
    size_t s = 0;

    for (size_t i = lstart; i < lend; i++)
        s += mLines[i].size();

    result.reserve(s + s / 8);

    while (istart < iend || lstart < lend)
    {
        if (lstart >= (int)mLines.size())
            break;

        auto& line = mLines[lstart];
        if (istart < (int)line.size())
        {
            result += line[istart].mChar;
            istart++;
        }
        else
        {
            istart = 0;
            ++lstart;
            result += '\n';
        }
    }

    return result;
}

TextEditor::Coordinates TextEditor::GetActualCursorCoordinates() const
{
    return SanitizeCoordinates(mState.mCursorPosition);
}

TextEditor::Coordinates TextEditor::SanitizeCoordinates(const Coordinates& aValue) const
{
    auto line = aValue.mLine;
    auto column = aValue.mColumn;
    if (line >= (int)mLines.size())
    {
        if (mLines.empty())
        {
            line = 0;
            column = 0;
        }
        else
        {
            line = (int)mLines.size() - 1;
            column = GetLineMaxColumn(line);
        }
        return Coordinates(line, column);
    }
    else
    {
        column = mLines.empty() ? 0 : std::min(column, GetLineMaxColumn(line));
        return Coordinates(line, column);
    }
}

static int UTF8CharLength(char c)
{
    if ((c & 0xFE) == 0xFC)
        return 6;
    if ((c & 0xFC) == 0xF8)
        return 5;
    if ((c & 0xF8) == 0xF0)
        return 4;
    else if ((c & 0xF0) == 0xE0)
        return 3;
    else if ((c & 0xE0) == 0xC0)
        return 2;
    return 1;
}

static inline int ImTextCharToUtf8(char* buf, int buf_size, unsigned int c)
{
    if (c < 0x80)
    {
        buf[0] = (char)c;
        return 1;
    }
    if (c < 0x800)
    {
        if (buf_size < 2) return 0;
        buf[0] = (char)(0xc0 + (c >> 6));
        buf[1] = (char)(0x80 + (c & 0x3f));
        return 2;
    }
    if (c >= 0xdc00 && c < 0xe000)
    {
        return 0;
    }
    if (c >= 0xd800 && c < 0xdc00)
    {
        if (buf_size < 4) return 0;
        buf[0] = (char)(0xf0 + (c >> 18));
        buf[1] = (char)(0x80 + ((c >> 12) & 0x3f));
        buf[2] = (char)(0x80 + ((c >> 6) & 0x3f));
        buf[3] = (char)(0x80 + ((c) & 0x3f));
        return 4;
    }
    //else if (c < 0x10000)
    {
        if (buf_size < 3) return 0;
        buf[0] = (char)(0xe0 + (c >> 12));
        buf[1] = (char)(0x80 + ((c >> 6) & 0x3f));
        buf[2] = (char)(0x80 + ((c) & 0x3f));
        return 3;
    }
}

void TextEditor::Advance(Coordinates& aCoordinates) const
{
    if (aCoordinates.mLine < (int)mLines.size())
    {
        auto& line = mLines[aCoordinates.mLine];
        auto cindex = GetCharacterIndex(aCoordinates);

        if (cindex + 1 < (int)line.size())
        {
            auto delta = UTF8CharLength(line[cindex].mChar);
            cindex = std::min(cindex + delta, (int)line.size() - 1);
        }
        else
        {
            ++aCoordinates.mLine;
            cindex = 0;
        }
        aCoordinates.mColumn = GetCharacterColumn(aCoordinates.mLine, cindex);
    }
}

void TextEditor::DeleteRange(const Coordinates& aStart, const Coordinates& aEnd)
{
    assert(aEnd >= aStart);

    //printf("D(%d.%d)-(%d.%d)\n", aStart.mLine, aStart.mColumn, aEnd.mLine, aEnd.mColumn);

    if (aEnd == aStart)
        return;

    auto start = GetCharacterIndex(aStart);
    auto end = GetCharacterIndex(aEnd);

    if (aStart.mLine == aEnd.mLine)
    {
        auto& line = mLines[aStart.mLine];
        auto n = GetLineMaxColumn(aStart.mLine);
        if (aEnd.mColumn >= n)
            line.erase(line.begin() + start, line.end());
        else
            line.erase(line.begin() + start, line.begin() + end);
    }
    else
    {
        auto& firstLine = mLines[aStart.mLine];
        auto& lastLine = mLines[aEnd.mLine];

        firstLine.erase(firstLine.begin() + start, firstLine.end());
        lastLine.erase(lastLine.begin(), lastLine.begin() + end);

        if (aStart.mLine < aEnd.mLine)
            firstLine.insert(firstLine.end(), lastLine.begin(), lastLine.end());

        if (aStart.mLine < aEnd.mLine)
            RemoveLine(aStart.mLine + 1, aEnd.mLine + 1);
    }

    mTextChanged = true;
}

int TextEditor::InsertTextAt(Coordinates& /* inout */ aWhere, const char* aValue)
{
    int cindex = GetCharacterIndex(aWhere);
    int totalLines = 0;
    while (*aValue != '\0')
    {
        assert(!mLines.empty());

        if (*aValue == '\r')
        {
            // skip
            ++aValue;
        }
        else if (*aValue == '\n')
        {
            if (cindex < (int)mLines[aWhere.mLine].size())
            {
                auto& newLine = InsertLine(aWhere.mLine + 1);
                auto& line = mLines[aWhere.mLine];
                newLine.insert(newLine.begin(), line.begin() + cindex, line.end());
                line.erase(line.begin() + cindex, line.end());
            }
            else
            {
                InsertLine(aWhere.mLine + 1);
            }
            ++aWhere.mLine;
            aWhere.mColumn = 0;
            cindex = 0;
            ++totalLines;
            ++aValue;
        }
        else
        {
            auto& line = mLines[aWhere.mLine];
            auto d = UTF8CharLength(*aValue);
            while (d-- > 0 && *aValue != '\0')
                line.insert(line.begin() + cindex++, Glyph(*aValue++, PaletteIndex::Default));
            ++aWhere.mColumn;
        }

        mTextChanged = true;
    }

    return totalLines;
}

void TextEditor::AddUndo(UndoRecord& aValue)
{
    mUndoBuffer.resize((size_t)(mUndoIndex + 1));
    mUndoBuffer.back() = aValue;
    ++mUndoIndex;
}

TextEditor::Coordinates TextEditor::ScreenPosToCoordinates(const ImVec2& aPosition) const
{
    ImVec2 origin = ImGui::GetCursorScreenPos();
    ImVec2 local(aPosition.x - origin.x, aPosition.y - origin.y);

    int lineNo = std::max(0, (int)floor(local.y / mCharAdvance.y));

    int columnCoord = 0;

    if (lineNo >= 0 && lineNo < (int)mLines.size())
    {
        auto& line = mLines.at(lineNo);

        int columnIndex = 0;
        float columnX = 0.0f;

        while ((size_t)columnIndex < line.size())
        {
            float columnWidth = 0.0f;

            if (line[columnIndex].mChar == '\t')
            {
                float spaceSize = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, " ").x;
                float oldX = columnX;
                float newColumnX = (1.0f + std::floor((1.0f + columnX) / (4.f * spaceSize))) * (4.f * spaceSize);
                columnWidth = newColumnX - oldX;
                if (mTextStart + columnX + columnWidth * 0.5f > local.x)
                    break;
                columnX = newColumnX;
                columnCoord = (columnCoord / 4.f) * 4.f + 4.f;
                columnIndex++;
            }
            else
            {
                char buf[7];
                auto d = UTF8CharLength(line[columnIndex].mChar);
                int i = 0;
                while (i < 6 && d-- > 0)
                    buf[i++] = line[columnIndex++].mChar;
                buf[i] = '\0';
                columnWidth = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, buf).x;
                if (mTextStart + columnX + columnWidth * 0.5f > local.x)
                    break;
                columnX += columnWidth;
                columnCoord++;
            }
        }
    }

    return SanitizeCoordinates(Coordinates(lineNo, columnCoord));
}

TextEditor::Coordinates TextEditor::FindWordStart(const Coordinates& aFrom) const
{
    Coordinates at = aFrom;
    if (at.mLine >= (int)mLines.size())
        return at;

    auto& line = mLines[at.mLine];
    auto cindex = GetCharacterIndex(at);

    if (cindex >= (int)line.size())
        return at;

    while (cindex > 0 && isspace(line[cindex].mChar))
        --cindex;

    auto cstart = (PaletteIndex)line[cindex].mColorIndex;
    while (cindex > 0)
    {
        auto c = line[cindex].mChar;
        if ((c & 0xC0) != 0x80)	// not UTF code sequence 10xxxxxx
        {
            if (c <= 32 && isspace(c))
            {
                cindex++;
                break;
            }
            if (cstart != (PaletteIndex)line[size_t(cindex - 1)].mColorIndex)
                break;
        }
        --cindex;
    }
    return Coordinates(at.mLine, GetCharacterColumn(at.mLine, cindex));
}

TextEditor::Coordinates TextEditor::FindWordEnd(const Coordinates& aFrom) const
{
    Coordinates at = aFrom;
    if (at.mLine >= (int)mLines.size())
        return at;

    auto& line = mLines[at.mLine];
    auto cindex = GetCharacterIndex(at);

    if (cindex >= (int)line.size())
        return at;

    bool prevspace = (bool)isspace(line[cindex].mChar);
    auto cstart = (PaletteIndex)line[cindex].mColorIndex;
    while (cindex < (int)line.size())
    {
        auto c = line[cindex].mChar;
        auto d = UTF8CharLength(c);
        if (cstart != (PaletteIndex)line[cindex].mColorIndex)
            break;

        if (prevspace != !!isspace(c))
        {
            if (isspace(c))
                while (cindex < (int)line.size() && isspace(line[cindex].mChar))
                    ++cindex;
            break;
        }
        cindex += d;
    }
    return Coordinates(aFrom.mLine, GetCharacterColumn(aFrom.mLine, cindex));
}

TextEditor::Coordinates TextEditor::FindNextWord(const Coordinates& aFrom) const
{
    Coordinates at = aFrom;
    if (at.mLine >= (int)mLines.size())
        return at;

    // skip to the next non-word character
    auto cindex = GetCharacterIndex(aFrom);
    bool isword = false;
    bool skip = false;
    if (cindex < (int)mLines[at.mLine].size())
    {
        auto& line = mLines[at.mLine];
        isword = isalnum(line[cindex].mChar);
        skip = isword;
    }

    while (!isword || skip)
    {
        if (at.mLine >= mLines.size())
        {
            auto l = std::max(0, (int)mLines.size() - 1);
            return Coordinates(l, GetLineMaxColumn(l));
        }

        auto& line = mLines[at.mLine];
        if (cindex < (int)line.size())
        {
            isword = isalnum(line[cindex].mChar);

            if (isword && !skip)
                return Coordinates(at.mLine, GetCharacterColumn(at.mLine, cindex));

            if (!isword)
                skip = false;

            cindex++;
        }
        else
        {
            cindex = 0;
            ++at.mLine;
            skip = false;
            isword = false;
        }
    }

    return at;
}

int TextEditor::GetCharacterIndex(const Coordinates& aCoordinates) const
{
    if (aCoordinates.mLine >= mLines.size())
        return -1;
    auto& line = mLines[aCoordinates.mLine];
    int c = 0;
    int i = 0;
    for (; i < line.size() && c < aCoordinates.mColumn;)
    {
        if (line[i].mChar == '\t')
            c = (c / 4.f) * 4.f + 4.f;
        else
            ++c;
        i += UTF8CharLength(line[i].mChar);
    }
    return i;
}

int TextEditor::GetCharacterColumn(int aLine, int aIndex) const
{
    if (aLine >= mLines.size())
        return 0;
    auto& line = mLines[aLine];
    int col = 0;
    int i = 0;
    while (i < aIndex && i < (int)line.size())
    {
        auto c = line[i].mChar;
        i += UTF8CharLength(c);
        if (c == '\t')
            col = (col / 4.f) * 4.f + 4.f;
        else
            col++;
    }
    return col;
}

int TextEditor::GetLineCharacterCount(int aLine) const
{
    if (aLine >= mLines.size())
        return 0;
    auto& line = mLines[aLine];
    int c = 0;
    for (unsigned i = 0; i < line.size(); c++)
        i += UTF8CharLength(line[i].mChar);
    return c;
}

int TextEditor::GetLineMaxColumn(int aLine) const
{
    if (aLine >= mLines.size())
        return 0;
    auto& line = mLines[aLine];
    int col = 0;
    for (unsigned i = 0; i < line.size(); )
    {
        auto c = line[i].mChar;
        if (c == '\t')
            col = (col / 4.f) * 4.f + 4.f;
        else
            col++;
        i += UTF8CharLength(c);
    }
    return col;
}

bool TextEditor::IsOnWordBoundary(const Coordinates& aAt) const
{
    if (aAt.mLine >= (int)mLines.size() || aAt.mColumn == 0)
        return true;

    auto& line = mLines[aAt.mLine];
    auto cindex = GetCharacterIndex(aAt);
    if (cindex >= (int)line.size())
        return true;

    return line[cindex].mColorIndex != line[size_t(cindex - 1)].mColorIndex;
}

void TextEditor::RemoveLine(int aStart, int aEnd)
{
    assert(aEnd >= aStart);
    assert(mLines.size() > (size_t)(aEnd - aStart));

    ErrorMarkers etmp;
    for (auto& i : mErrorMarkers)
    {
        ErrorMarkers::value_type e(i.first >= aStart ? i.first - 1 : i.first, i.second);
        if (e.first >= aStart && e.first <= aEnd)
            continue;
        etmp.insert(e);
    }
    mErrorMarkers = std::move(etmp);

    mLines.erase(mLines.begin() + aStart, mLines.begin() + aEnd);
    assert(!mLines.empty());

    mTextChanged = true;
}

void TextEditor::RemoveLine(int aIndex)
{
    assert(mLines.size() > 1);

    ErrorMarkers etmp;
    for (auto& i : mErrorMarkers)
    {
        ErrorMarkers::value_type e(i.first > aIndex ? i.first - 1 : i.first, i.second);
        if (e.first - 1 == aIndex)
            continue;
        etmp.insert(e);
    }
    mErrorMarkers = std::move(etmp);

    mLines.erase(mLines.begin() + aIndex);
    assert(!mLines.empty());

    mTextChanged = true;
}

std::vector<TextEditor::Glyph>& TextEditor::InsertLine(int aIndex)
{
    auto& result = *mLines.insert(mLines.begin() + aIndex, std::vector<TextEditor::Glyph>());

    ErrorMarkers etmp;
    for (auto& i : mErrorMarkers)
        etmp.insert(ErrorMarkers::value_type(i.first >= aIndex ? i.first + 1 : i.first, i.second));
    mErrorMarkers = std::move(etmp);

    return result;
}

std::string TextEditor::GetWordAt(const Coordinates& aCoords) const
{
    auto start = FindWordStart(aCoords);
    auto end = FindWordEnd(aCoords);

    std::string r;

    auto istart = GetCharacterIndex(start);
    auto iend = GetCharacterIndex(end);

    for (auto it = istart; it < iend; ++it)
        r.push_back(mLines[aCoords.mLine][it].mChar);

    return r;
}

ImColor TextEditor::GetGlyphColor(const Glyph& aGlyph) const
{
    if (aGlyph.mComment)
        return mPalette[(int)PaletteIndex::Comment];
    if (aGlyph.mMultiLineComment)
        return mPalette[(int)PaletteIndex::MultiLineComment];
    auto const color = mPalette[(int)aGlyph.mColorIndex];

    return color;
}

void TextEditor::HandleKeyboardInputs()
{
    ImGuiIO& io = ImGui::GetIO();
    auto shift = io.KeyShift;
    auto ctrl = io.ConfigMacOSXBehaviors ? io.KeySuper : io.KeyCtrl;
    auto alt = io.ConfigMacOSXBehaviors ? io.KeyCtrl : io.KeyAlt;

    if (ImGui::IsWindowFocused())
    {
        if (ImGui::IsWindowHovered())
            ImGui::SetMouseCursor(ImGuiMouseCursor_TextInput);
        //ImGui::CaptureKeyboardFromApp(true);

        io.WantCaptureKeyboard = true;
        io.WantTextInput = true;

        if (ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Z)))
            Undo();
        else if (ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Y)))
            Redo();
        else if (!ctrl && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_UpArrow)))
            MoveUp(1, shift);
        else if (!ctrl && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_DownArrow)))
            MoveDown(1, shift);
        else if (!alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_LeftArrow)))
            MoveLeft(1, shift, ctrl);
        else if (!alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_RightArrow)))
            MoveRight(1, shift, ctrl);
        else if (!ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Backspace)))
            Backspace();
        else if (ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_C)))
            Copy();
        else if (!ctrl && shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Insert)))
            Paste();
        else if (ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_V)))
            Paste();
        else if (ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_A)))
            SetSelection(Coordinates(0, 0), Coordinates((int)mLines.size(), 0));
        else if (!ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Enter)))
            EnterCharacter('\n', false);
        else if (!ctrl && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Tab)))
            EnterCharacter('\t', shift);

        if (!io.InputQueueCharacters.empty())
        {
            for (int i = 0; i < io.InputQueueCharacters.Size; i++)
            {
                auto c = io.InputQueueCharacters[i];
                if (c != 0 && (c == '\n' || c >= 32))
                    EnterCharacter(c, shift);
            }
            io.InputQueueCharacters.resize(0);
        }
    }
}

void TextEditor::HandleMouseInputs()
{
    ImGuiIO& io = ImGui::GetIO();
    auto shift = io.KeyShift;
    auto ctrl = io.ConfigMacOSXBehaviors ? io.KeySuper : io.KeyCtrl;
    auto alt = io.ConfigMacOSXBehaviors ? io.KeyCtrl : io.KeyAlt;

    if (ImGui::IsWindowHovered())
    {
        if (!shift && !alt)
        {
            auto click = ImGui::IsMouseClicked(0);
            auto doubleClick = ImGui::IsMouseDoubleClicked(0);
            auto t = ImGui::GetTime();
            auto tripleClick = click && !doubleClick && (mLastClick != -1.0f && (t - mLastClick) < io.MouseDoubleClickTime);

            /*
            Left mouse button triple click
            */

            if (tripleClick)
            {
                if (!ctrl)
                {
                    mState.mCursorPosition = mInteractiveStart = mInteractiveEnd = ScreenPosToCoordinates(ImGui::GetMousePos());
                    mSelectionMode = SelectionMode::Line;
                    SetSelection(mInteractiveStart, mInteractiveEnd, mSelectionMode);
                }

                mLastClick = -1.0f;
            }

            /*
            Left mouse button double click
            */

            else if (doubleClick)
            {
                if (!ctrl)
                {
                    mState.mCursorPosition = mInteractiveStart = mInteractiveEnd = ScreenPosToCoordinates(ImGui::GetMousePos());
                    if (mSelectionMode == SelectionMode::Line)
                        mSelectionMode = SelectionMode::Normal;
                    else
                        mSelectionMode = SelectionMode::Word;
                    SetSelection(mInteractiveStart, mInteractiveEnd, mSelectionMode);
                }

                mLastClick = (float)ImGui::GetTime();
            }

            /*
            Left mouse button click
            */
            else if (click)
            {
                mState.mCursorPosition = mInteractiveStart = mInteractiveEnd = ScreenPosToCoordinates(ImGui::GetMousePos());
                if (ctrl)
                    mSelectionMode = SelectionMode::Word;
                else
                    mSelectionMode = SelectionMode::Normal;
                SetSelection(mInteractiveStart, mInteractiveEnd, mSelectionMode);

                mLastClick = (float)ImGui::GetTime();
            }
            // Mouse left button dragging (=> update selection)
            else if (ImGui::IsMouseDragging(0) && ImGui::IsMouseDown(0))
            {
                io.WantCaptureMouse = true;
                mState.mCursorPosition = mInteractiveEnd = ScreenPosToCoordinates(ImGui::GetMousePos());
                SetSelection(mInteractiveStart, mInteractiveEnd, mSelectionMode);
            }
        }
    }
}

void TextEditor::Render()
{
    /* Compute mCharAdvance regarding to scaled font size (Ctrl + mouse wheel)*/
    const float fontSize = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, "#", nullptr, nullptr).x;
    mCharAdvance = ImVec2(fontSize, ImGui::GetTextLineHeightWithSpacing() * mLineSpacing);

    assert(mLineBuffer.empty());

    auto contentSize = ImGui::GetWindowContentRegionMax();
    auto drawList = ImGui::GetWindowDrawList();
    float longest(mTextStart);

    if (mScrollToTop)
    {
        mScrollToTop = false;
        ImGui::SetScrollY(0.f);
    }

    ImVec2 cursorScreenPos = ImGui::GetCursorScreenPos();
    auto scrollX = ImGui::GetScrollX();
    auto scrollY = ImGui::GetScrollY();

    auto lineNo = (int)floor(scrollY / mCharAdvance.y);
    auto globalLineMax = (int)mLines.size();
    auto lineMax = std::max(0, std::min((int)mLines.size() - 1, lineNo + (int)floor((scrollY + contentSize.y) / mCharAdvance.y)));

    // Deduce mTextStart by evaluating mLines size (global lineMax) plus two spaces as text width
    char buf[16];
    snprintf(buf, 16, " %d ", globalLineMax);
    mTextStart = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, buf, nullptr, nullptr).x + 10;

    if (!mLines.empty())
    {
        float spaceSize = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, " ", nullptr, nullptr).x;

        while (lineNo <= lineMax)
        {
            ImVec2 lineStartScreenPos = ImVec2(cursorScreenPos.x, cursorScreenPos.y + lineNo * mCharAdvance.y);
            ImVec2 textScreenPos = ImVec2(lineStartScreenPos.x + mTextStart, lineStartScreenPos.y);

            auto& line = mLines[lineNo];
            longest = std::max(mTextStart + TextDistanceToLineStart(Coordinates(lineNo, GetLineMaxColumn(lineNo))), longest);
            auto columnNo = 0;
            Coordinates lineStartCoord(lineNo, 0);
            Coordinates lineEndCoord(lineNo, GetLineMaxColumn(lineNo));

            // Draw selection for the current line
            float sstart = -1.0f;
            float ssend = -1.0f;

            assert(mState.mSelectionStart <= mState.mSelectionEnd);
            if (mState.mSelectionStart <= lineEndCoord)
                sstart = mState.mSelectionStart > lineStartCoord ? TextDistanceToLineStart(mState.mSelectionStart) : 0.0f;
            if (mState.mSelectionEnd > lineStartCoord)
                ssend = TextDistanceToLineStart(mState.mSelectionEnd < lineEndCoord ? mState.mSelectionEnd : lineEndCoord);

            if (mState.mSelectionEnd.mLine > lineNo)
                ssend += mCharAdvance.x;

            if (sstart != -1 && ssend != -1 && sstart < ssend)
            {
                ImVec2 vstart(lineStartScreenPos.x + mTextStart + sstart, lineStartScreenPos.y);
                ImVec2 vend(lineStartScreenPos.x + mTextStart + ssend, lineStartScreenPos.y + mCharAdvance.y);
                drawList->AddRectFilled(vstart, vend, mPalette[(int)PaletteIndex::Selection]);
            }

            auto start = ImVec2(lineStartScreenPos.x + scrollX, lineStartScreenPos.y);

            // Draw error markers
            auto errorIt = mErrorMarkers.find(lineNo + 1);
            if (errorIt != mErrorMarkers.end())
            {
                auto end = ImVec2(lineStartScreenPos.x + contentSize.x + 2.0f * scrollX, lineStartScreenPos.y + mCharAdvance.y);
                drawList->AddRectFilled(start, end, mPalette[(int)PaletteIndex::ErrorMarker]);

                if (ImGui::IsMouseHoveringRect(lineStartScreenPos, end))
                {
                    ImGui::BeginTooltip();
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
                    ImGui::Text("Error at line %d:", errorIt->first);
                    ImGui::PopStyleColor();
                    ImGui::Separator();
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.2f, 1.0f));
                    ImGui::Text("%s", errorIt->second.c_str());
                    ImGui::PopStyleColor();
                    ImGui::EndTooltip();
                }
            }

            // Draw line number (right aligned)
            snprintf(buf, 16, "%d  ", lineNo + 1);

            auto lineNoWidth = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, buf, nullptr, nullptr).x;
            drawList->AddText(ImVec2(lineStartScreenPos.x + mTextStart - lineNoWidth, lineStartScreenPos.y), mPalette[(int)PaletteIndex::LineNumber], buf);

            if (mState.mCursorPosition.mLine == lineNo)
            {
                auto focused = ImGui::IsWindowFocused();

                // Highlight the current line (where the cursor is)
                if (!HasSelection())
                {
                    auto end = ImVec2(start.x + contentSize.x + scrollX, start.y + mCharAdvance.y);
                    drawList->AddRectFilled(start, end, mPalette[(int)(focused ? PaletteIndex::CurrentLineFill : PaletteIndex::CurrentLineFillInactive)]);
                    drawList->AddRect(start, end, mPalette[(int)PaletteIndex::CurrentLineEdge], 1.0f);
                }

                // Render the cursor
                if (focused)
                {
                    auto timeEnd = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
                    auto elapsed = timeEnd - mStartTime;
                    if (elapsed > 400)
                    {
                        float cx = TextDistanceToLineStart(mState.mCursorPosition);

                        ImVec2 cstart(textScreenPos.x + cx, lineStartScreenPos.y);
                        ImVec2 cend(textScreenPos.x + cx + 1.f, lineStartScreenPos.y + mCharAdvance.y);
                        drawList->AddRectFilled(cstart, cend, mPalette[(int)PaletteIndex::Cursor]);
                        if (elapsed > 800)
                            mStartTime = timeEnd;
                    }
                }
            }

            // Render colorized text
            auto prevColor = line.empty() ? mPalette[(int)PaletteIndex::Default] : GetGlyphColor(line[0]);
            ImVec2 bufferOffset;

            for (int i = 0; i < line.size();)
            {
                auto& glyph = line[i];
                auto color = GetGlyphColor(glyph);

                if (((color.Value.x != prevColor.Value.x || color.Value.y != prevColor.Value.y || color.Value.z != prevColor.Value.z || color.Value.w != prevColor.Value.w) || glyph.mChar == '\t' || glyph.mChar == ' ') && !mLineBuffer.empty())
                {
                    const ImVec2 newOffset(textScreenPos.x + bufferOffset.x, textScreenPos.y + bufferOffset.y);
                    drawList->AddText(newOffset, prevColor, mLineBuffer.c_str());
                    auto textSize = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, mLineBuffer.c_str(), nullptr, nullptr);
                    bufferOffset.x += textSize.x;
                    mLineBuffer.clear();
                }
                prevColor = color;

                if (glyph.mChar == '\t')
                {
                    auto oldX = bufferOffset.x;
                    bufferOffset.x = (1.0f + std::floor((1.0f + bufferOffset.x) / (4.f * spaceSize))) * (4.f * spaceSize);
                    ++i;
                }
                else if (glyph.mChar == ' ')
                {
                    bufferOffset.x += spaceSize;
                    i++;
                }
                else
                {
                    auto l = UTF8CharLength(glyph.mChar);
                    while (l-- > 0)
                        mLineBuffer.push_back(line[i++].mChar);
                }
                ++columnNo;
            }

            if (!mLineBuffer.empty())
            {
                const ImVec2 newOffset(textScreenPos.x + bufferOffset.x, textScreenPos.y + bufferOffset.y);
                drawList->AddText(newOffset, prevColor, mLineBuffer.c_str());
                mLineBuffer.clear();
            }

            ++lineNo;
        }
    }

    ImGui::Dummy(ImVec2((longest + 2), mLines.size() * mCharAdvance.y));
}

void TextEditor::Render(const char* aTitle, const ImVec2& aSize, bool aBorder)
{
    mWithinRender = true;
    mTextChanged = false;
    mCursorPositionChanged = false;

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::ColorConvertU32ToFloat4(mPalette[(int)PaletteIndex::Background]));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
    ImGui::BeginChild(aTitle, aSize, aBorder, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoMove);

    HandleKeyboardInputs();
    ImGui::PushAllowKeyboardFocus(true);

    HandleMouseInputs();

    ColorizeInternal();
    Render();

    ImGui::PopAllowKeyboardFocus();

    ImGui::EndChild();

    ImGui::PopStyleVar();
    ImGui::PopStyleColor();

    mWithinRender = false;
}

void TextEditor::SetText(const std::string& aText)
{
    mLines.clear();
    mLines.emplace_back(std::vector<TextEditor::Glyph>());
    for (auto chr : aText)
    {
        if (chr == '\r')
        {
            // ignore the carriage return character
        }
        else if (chr == '\n')
            mLines.emplace_back(std::vector<TextEditor::Glyph>());
        else
        {
            mLines.back().emplace_back(Glyph(chr, PaletteIndex::Default));
        }
    }

    mTextChanged = true;
    mScrollToTop = true;

    mUndoBuffer.clear();
    mUndoIndex = 0;

    Colorize();
}

void TextEditor::EnterCharacter(ImWchar aChar, bool aShift)
{
    UndoRecord u;

    u.mBefore = mState;

    if (HasSelection())
    {
        if (aChar == '\t' && mState.mSelectionStart.mLine != mState.mSelectionEnd.mLine)
        {

            auto start = mState.mSelectionStart;
            auto end = mState.mSelectionEnd;
            auto originalEnd = end;

            if (start > end)
                std::swap(start, end);
            start.mColumn = 0;
            //			end.mColumn = end.mLine < mLines.size() ? mLines[end.mLine].size() : 0;
            if (end.mColumn == 0 && end.mLine > 0)
                --end.mLine;
            if (end.mLine >= (int)mLines.size())
                end.mLine = mLines.empty() ? 0 : (int)mLines.size() - 1;
            end.mColumn = GetLineMaxColumn(end.mLine);

            //if (end.mColumn >= GetLineMaxColumn(end.mLine))
            //	end.mColumn = GetLineMaxColumn(end.mLine) - 1;

            u.mRemovedStart = start;
            u.mRemovedEnd = end;
            u.mRemoved = GetText(start, end);

            bool modified = false;

            for (int i = start.mLine; i <= end.mLine; i++)
            {
                auto& line = mLines[i];
                if (aShift)
                {
                    if (!line.empty())
                    {
                        if (line.front().mChar == '\t')
                        {
                            line.erase(line.begin());
                            modified = true;
                        }
                        else
                        {
                            for (int j = 0; j < 4.f && !line.empty() && line.front().mChar == ' '; j++)
                            {
                                line.erase(line.begin());
                                modified = true;
                            }
                        }
                    }
                }
                else
                {
                    line.insert(line.begin(), Glyph('\t', TextEditor::PaletteIndex::Background));
                    modified = true;
                }
            }

            if (modified)
            {
                start = Coordinates(start.mLine, GetCharacterColumn(start.mLine, 0));
                Coordinates rangeEnd;
                if (originalEnd.mColumn != 0)
                {
                    end = Coordinates(end.mLine, GetLineMaxColumn(end.mLine));
                    rangeEnd = end;
                    u.mAdded = GetText(start, end);
                }
                else
                {
                    end = Coordinates(originalEnd.mLine, 0);
                    rangeEnd = Coordinates(end.mLine - 1, GetLineMaxColumn(end.mLine - 1));
                    u.mAdded = GetText(start, rangeEnd);
                }

                u.mAddedStart = start;
                u.mAddedEnd = rangeEnd;
                u.mAfter = mState;

                mState.mSelectionStart = start;
                mState.mSelectionEnd = end;
                AddUndo(u);

                mTextChanged = true;
            }

            return;
        } // c == '\t'
        else
        {
            u.mRemoved = GetSelectedText();
            u.mRemovedStart = mState.mSelectionStart;
            u.mRemovedEnd = mState.mSelectionEnd;
            DeleteSelection();
        }
    } // HasSelection

    auto coord = GetActualCursorCoordinates();
    u.mAddedStart = coord;

    assert(!mLines.empty());

    if (aChar == '\n')
    {
        InsertLine(coord.mLine + 1);
        auto& line = mLines[coord.mLine];
        auto& newLine = mLines[coord.mLine + 1];

        if (mLanguageDefinition.mAutoIndentation)
            for (size_t it = 0; it < line.size() && isascii(line[it].mChar) && isblank(line[it].mChar); ++it)
                newLine.push_back(line[it]);

        const size_t whitespaceSize = newLine.size();
        auto cindex = GetCharacterIndex(coord);
        newLine.insert(newLine.end(), line.begin() + cindex, line.end());
        line.erase(line.begin() + cindex, line.begin() + line.size());
        SetCursorPosition(Coordinates(coord.mLine + 1, GetCharacterColumn(coord.mLine + 1, (int)whitespaceSize)));
        u.mAdded = (char)aChar;
    }
    else
    {
        char buf[7];
        int e = ImTextCharToUtf8(buf, 7, aChar);
        if (e > 0)
        {
            buf[e] = '\0';
            auto& line = mLines[coord.mLine];
            auto cindex = GetCharacterIndex(coord);

            for (auto p = buf; *p != '\0'; p++, ++cindex)
                line.insert(line.begin() + cindex, Glyph(*p, PaletteIndex::Default));
            u.mAdded = buf;

            SetCursorPosition(Coordinates(coord.mLine, GetCharacterColumn(coord.mLine, cindex)));
        }
        else
            return;
    }

    mTextChanged = true;

    u.mAddedEnd = GetActualCursorCoordinates();
    u.mAfter = mState;

    AddUndo(u);

    Colorize(coord.mLine - 1, 3);
}

void TextEditor::SetCursorPosition(const Coordinates& aPosition)
{
    if (mState.mCursorPosition != aPosition)
    {
        mState.mCursorPosition = aPosition;
        mCursorPositionChanged = true;
    }
}

void TextEditor::SetSelection(const Coordinates& aStart, const Coordinates& aEnd, SelectionMode aMode)
{
    auto oldSelStart = mState.mSelectionStart;
    auto oldSelEnd = mState.mSelectionEnd;

    mState.mSelectionStart = SanitizeCoordinates(aStart);
    mState.mSelectionEnd = SanitizeCoordinates(aEnd);
    if (mState.mSelectionStart > mState.mSelectionEnd)
        std::swap(mState.mSelectionStart, mState.mSelectionEnd);

    switch (aMode)
    {
    case TextEditor::SelectionMode::Normal:
        break;
    case TextEditor::SelectionMode::Word:
    {
        mState.mSelectionStart = FindWordStart(mState.mSelectionStart);
        if (!IsOnWordBoundary(mState.mSelectionEnd))
            mState.mSelectionEnd = FindWordEnd(FindWordStart(mState.mSelectionEnd));
        break;
    }
    case TextEditor::SelectionMode::Line:
    {
        const auto lineNo = mState.mSelectionEnd.mLine;
        const auto lineSize = (size_t)lineNo < mLines.size() ? mLines[lineNo].size() : 0;
        mState.mSelectionStart = Coordinates(mState.mSelectionStart.mLine, 0);
        mState.mSelectionEnd = Coordinates(lineNo, GetLineMaxColumn(lineNo));
        break;
    }
    default:
        break;
    }

    if (mState.mSelectionStart != oldSelStart ||
        mState.mSelectionEnd != oldSelEnd)
        mCursorPositionChanged = true;
}

void TextEditor::InsertText(const std::string& aValue)
{
    InsertText(aValue.c_str());
}

void TextEditor::InsertText(const char* aValue)
{
    if (aValue == nullptr)
        return;

    auto pos = GetActualCursorCoordinates();
    auto start = std::min(pos, mState.mSelectionStart);
    int totalLines = pos.mLine - start.mLine;

    totalLines += InsertTextAt(pos, aValue);

    SetSelection(pos, pos);
    SetCursorPosition(pos);
    Colorize(start.mLine - 1, totalLines + 2);
}

void TextEditor::DeleteSelection()
{
    assert(mState.mSelectionEnd >= mState.mSelectionStart);

    if (mState.mSelectionEnd == mState.mSelectionStart)
        return;

    DeleteRange(mState.mSelectionStart, mState.mSelectionEnd);

    SetSelection(mState.mSelectionStart, mState.mSelectionStart);
    SetCursorPosition(mState.mSelectionStart);
    Colorize(mState.mSelectionStart.mLine, 1);
}

void TextEditor::MoveUp(int aAmount, bool aSelect)
{
    auto oldPos = mState.mCursorPosition;
    mState.mCursorPosition.mLine = std::max(0, mState.mCursorPosition.mLine - aAmount);
    if (oldPos != mState.mCursorPosition)
    {
        if (aSelect)
        {
            if (oldPos == mInteractiveStart)
                mInteractiveStart = mState.mCursorPosition;
            else if (oldPos == mInteractiveEnd)
                mInteractiveEnd = mState.mCursorPosition;
            else
            {
                mInteractiveStart = mState.mCursorPosition;
                mInteractiveEnd = oldPos;
            }
        }
        else
            mInteractiveStart = mInteractiveEnd = mState.mCursorPosition;
        SetSelection(mInteractiveStart, mInteractiveEnd);
    }
}

void TextEditor::MoveDown(int aAmount, bool aSelect)
{
    assert(mState.mCursorPosition.mColumn >= 0);
    auto oldPos = mState.mCursorPosition;
    mState.mCursorPosition.mLine = std::max(0, std::min((int)mLines.size() - 1, mState.mCursorPosition.mLine + aAmount));

    if (mState.mCursorPosition != oldPos)
    {
        if (aSelect)
        {
            if (oldPos == mInteractiveEnd)
                mInteractiveEnd = mState.mCursorPosition;
            else if (oldPos == mInteractiveStart)
                mInteractiveStart = mState.mCursorPosition;
            else
            {
                mInteractiveStart = oldPos;
                mInteractiveEnd = mState.mCursorPosition;
            }
        }
        else
            mInteractiveStart = mInteractiveEnd = mState.mCursorPosition;
        SetSelection(mInteractiveStart, mInteractiveEnd);
    }
}

void TextEditor::MoveLeft(int aAmount, bool aSelect, bool aWordMode)
{
    if (mLines.empty())
        return;

    auto oldPos = mState.mCursorPosition;
    mState.mCursorPosition = GetActualCursorCoordinates();
    auto line = mState.mCursorPosition.mLine;
    auto cindex = GetCharacterIndex(mState.mCursorPosition);

    while (aAmount-- > 0)
    {
        if (cindex == 0)
        {
            if (line > 0)
            {
                --line;
                if ((int)mLines.size() > line)
                    cindex = (int)mLines[line].size();
                else
                    cindex = 0;
            }
        }
        else
        {
            --cindex;
            if (cindex > 0)
            {
                if ((int)mLines.size() > line)
                {
                    while (cindex > 0 && (mLines[line][cindex].mChar & 0xC0) == 0x80)
                        --cindex;
                }
            }
        }

        mState.mCursorPosition = Coordinates(line, GetCharacterColumn(line, cindex));
        if (aWordMode)
        {
            mState.mCursorPosition = FindWordStart(mState.mCursorPosition);
            cindex = GetCharacterIndex(mState.mCursorPosition);
        }
    }

    mState.mCursorPosition = Coordinates(line, GetCharacterColumn(line, cindex));

    assert(mState.mCursorPosition.mColumn >= 0);
    if (aSelect)
    {
        if (oldPos == mInteractiveStart)
            mInteractiveStart = mState.mCursorPosition;
        else if (oldPos == mInteractiveEnd)
            mInteractiveEnd = mState.mCursorPosition;
        else
        {
            mInteractiveStart = mState.mCursorPosition;
            mInteractiveEnd = oldPos;
        }
    }
    else
        mInteractiveStart = mInteractiveEnd = mState.mCursorPosition;
    SetSelection(mInteractiveStart, mInteractiveEnd, aSelect && aWordMode ? SelectionMode::Word : SelectionMode::Normal);
}

void TextEditor::MoveRight(int aAmount, bool aSelect, bool aWordMode)
{
    auto oldPos = mState.mCursorPosition;

    if (mLines.empty() || oldPos.mLine >= mLines.size())
        return;

    auto cindex = GetCharacterIndex(mState.mCursorPosition);
    while (aAmount-- > 0)
    {
        auto lindex = mState.mCursorPosition.mLine;
        auto& line = mLines[lindex];

        if (cindex >= line.size())
        {
            if (mState.mCursorPosition.mLine < mLines.size() - 1)
            {
                mState.mCursorPosition.mLine = std::max(0, std::min((int)mLines.size() - 1, mState.mCursorPosition.mLine + 1));
                mState.mCursorPosition.mColumn = 0;
            }
            else
                return;
        }
        else
        {
            cindex += UTF8CharLength(line[cindex].mChar);
            mState.mCursorPosition = Coordinates(lindex, GetCharacterColumn(lindex, cindex));
            if (aWordMode)
                mState.mCursorPosition = FindNextWord(mState.mCursorPosition);
        }
    }

    if (aSelect)
    {
        if (oldPos == mInteractiveEnd)
            mInteractiveEnd = SanitizeCoordinates(mState.mCursorPosition);
        else if (oldPos == mInteractiveStart)
            mInteractiveStart = mState.mCursorPosition;
        else
        {
            mInteractiveStart = oldPos;
            mInteractiveEnd = mState.mCursorPosition;
        }
    }
    else
        mInteractiveStart = mInteractiveEnd = mState.mCursorPosition;
    SetSelection(mInteractiveStart, mInteractiveEnd, aSelect && aWordMode ? SelectionMode::Word : SelectionMode::Normal);
}

void TextEditor::Backspace()
{
    if (mLines.empty())
        return;

    UndoRecord u;
    u.mBefore = mState;

    if (HasSelection())
    {
        u.mRemoved = GetSelectedText();
        u.mRemovedStart = mState.mSelectionStart;
        u.mRemovedEnd = mState.mSelectionEnd;

        DeleteSelection();
    }
    else
    {
        auto pos = GetActualCursorCoordinates();
        SetCursorPosition(pos);

        if (mState.mCursorPosition.mColumn == 0)
        {
            if (mState.mCursorPosition.mLine == 0)
                return;

            u.mRemoved = '\n';
            u.mRemovedStart = u.mRemovedEnd = Coordinates(pos.mLine - 1, GetLineMaxColumn(pos.mLine - 1));
            Advance(u.mRemovedEnd);

            auto& line = mLines[mState.mCursorPosition.mLine];
            auto& prevLine = mLines[mState.mCursorPosition.mLine - 1];
            auto prevSize = GetLineMaxColumn(mState.mCursorPosition.mLine - 1);
            prevLine.insert(prevLine.end(), line.begin(), line.end());

            ErrorMarkers etmp;
            for (auto& i : mErrorMarkers)
                etmp.insert(ErrorMarkers::value_type(i.first - 1 == mState.mCursorPosition.mLine ? i.first - 1 : i.first, i.second));
            mErrorMarkers = std::move(etmp);

            RemoveLine(mState.mCursorPosition.mLine);
            --mState.mCursorPosition.mLine;
            mState.mCursorPosition.mColumn = prevSize;
        }
        else
        {
            auto& line = mLines[mState.mCursorPosition.mLine];
            auto cindex = GetCharacterIndex(pos) - 1;
            auto cend = cindex + 1;
            while (cindex > 0 && (line[cindex].mChar & 0xC0) == 0x80)
                --cindex;

            //if (cindex > 0 && UTF8CharLength(line[cindex].mChar) > 1)
            //	--cindex;

            u.mRemovedStart = u.mRemovedEnd = GetActualCursorCoordinates();
            --u.mRemovedStart.mColumn;
            --mState.mCursorPosition.mColumn;

            while (cindex < line.size() && cend-- > cindex)
            {
                u.mRemoved += line[cindex].mChar;
                line.erase(line.begin() + cindex);
            }
        }

        mTextChanged = true;

        Colorize(mState.mCursorPosition.mLine, 1);
    }

    u.mAfter = mState;
    AddUndo(u);
}

bool TextEditor::HasSelection() const
{
    return mState.mSelectionEnd > mState.mSelectionStart;
}

void TextEditor::Copy()
{
    if (HasSelection())
    {
        ImGui::SetClipboardText(GetSelectedText().c_str());
    }
}

void TextEditor::Paste()
{
    auto clipText = ImGui::GetClipboardText();
    if (clipText != nullptr && strlen(clipText) > 0)
    {
        UndoRecord u;
        u.mBefore = mState;

        if (HasSelection())
        {
            u.mRemoved = GetSelectedText();
            u.mRemovedStart = mState.mSelectionStart;
            u.mRemovedEnd = mState.mSelectionEnd;
            DeleteSelection();
        }

        u.mAdded = clipText;
        u.mAddedStart = GetActualCursorCoordinates();

        InsertText(clipText);

        u.mAddedEnd = GetActualCursorCoordinates();
        u.mAfter = mState;
        AddUndo(u);
    }
}

void TextEditor::Undo(int aSteps)
{
    while (mUndoIndex > 0 && aSteps-- > 0)
        mUndoBuffer[--mUndoIndex].Undo(this);
}

void TextEditor::Redo(int aSteps)
{
    while (mUndoIndex < (int)mUndoBuffer.size() && aSteps-- > 0)
        mUndoBuffer[mUndoIndex++].Redo(this);
}

std::string TextEditor::GetText() const
{
    return GetText(Coordinates(), Coordinates((int)mLines.size(), 0));
}

std::string TextEditor::GetSelectedText() const
{
    return GetText(mState.mSelectionStart, mState.mSelectionEnd);
}

void TextEditor::Colorize(int aFromLine, int aLines)
{
    int toLine = aLines == -1 ? (int)mLines.size() : std::min((int)mLines.size(), aFromLine + aLines);
    mColorRangeMin = std::min(mColorRangeMin, aFromLine);
    mColorRangeMax = std::max(mColorRangeMax, toLine);
    mColorRangeMin = std::max(0, mColorRangeMin);
    mColorRangeMax = std::max(mColorRangeMin, mColorRangeMax);
    mCheckComments = true;
}

void TextEditor::ColorizeRange(int aFromLine, int aToLine)
{
    if (mLines.empty() || aFromLine >= aToLine)
        return;

    std::string buffer;
    std::cmatch results;
    std::string id;

    int endLine = std::max(0, std::min((int)mLines.size(), aToLine));
    for (int i = aFromLine; i < endLine; ++i)
    {
        auto& line = mLines[i];

        if (line.empty())
            continue;

        buffer.resize(line.size());
        for (size_t j = 0; j < line.size(); ++j)
        {
            auto& col = line[j];
            buffer[j] = col.mChar;
            col.mColorIndex = PaletteIndex::Default;
        }

        const char* bufferBegin = &buffer.front();
        const char* bufferEnd = bufferBegin + buffer.size();

        auto last = bufferEnd;

        for (auto first = bufferBegin; first != last; )
        {
            const char* token_begin = nullptr;
            const char* token_end = nullptr;
            PaletteIndex token_color = PaletteIndex::Default;

            bool hasTokenizeResult = false;

            if (mLanguageDefinition.mTokenize != nullptr)
            {
                if (mLanguageDefinition.mTokenize(first, last, token_begin, token_end, token_color))
                    hasTokenizeResult = true;
            }

            if (hasTokenizeResult == false)
            {
                // todo : remove
                //printf("using regex for %.*s\n", first + 10 < last ? 10 : int(last - first), first);

                for (auto& p : mRegexList)
                {
                    if (std::regex_search(first, last, results, p.first, std::regex_constants::match_continuous))
                    {
                        hasTokenizeResult = true;

                        auto& v = *results.begin();
                        token_begin = v.first;
                        token_end = v.second;
                        token_color = p.second;
                        break;
                    }
                }
            }

            if (hasTokenizeResult == false)
            {
                first++;
            }
            else
            {
                const size_t token_length = token_end - token_begin;

                if (token_color == PaletteIndex::Identifier)
                {
                    id.assign(token_begin, token_end);

                    if (!mLanguageDefinition.mCaseSensitive)
                        std::transform(id.begin(), id.end(), id.begin(), ::toupper);

                    if (mLanguageDefinition.mPurpleKeywords.count(id) != 0)
                        token_color = PaletteIndex::PurpleKeyword;
                    else if (mLanguageDefinition.mBlueKeywords.count(id) != 0)
                        token_color = PaletteIndex::BlueKeyword;
                    else if (mLanguageDefinition.mKnownFunctions.count(id) != 0)
                        token_color = PaletteIndex::KnownFunction;
                }

                for (size_t j = 0; j < token_length; ++j)
                    line[(token_begin - bufferBegin) + j].mColorIndex = token_color;

                first = token_end;
            }
        }
    }
}

void TextEditor::ColorizeInternal()
{
    if (mLines.empty())
        return;

    if (mCheckComments)
    {
        auto endLine = mLines.size();
        auto endIndex = 0;
        auto commentStartLine = endLine;
        auto commentStartIndex = endIndex;
        auto withinString = false;
        auto withinSingleLineComment = false;
        auto firstChar = true;
        auto concatenate = false;
        auto currentLine = 0;
        auto currentIndex = 0;
        while (currentLine < endLine || currentIndex < endIndex)
        {
            auto& line = mLines[currentLine];

            if (currentIndex == 0 && !concatenate)
            {
                withinSingleLineComment = false;
                firstChar = true;
            }

            concatenate = false;

            if (!line.empty())
            {
                auto& g = line[currentIndex];
                auto c = g.mChar;

                if (c != mLanguageDefinition.mPreprocChar && !isspace(c))
                    firstChar = false;

                if (currentIndex == (int)line.size() - 1 && line[line.size() - 1].mChar == '\\')
                    concatenate = true;

                bool inComment = (commentStartLine < currentLine || (commentStartLine == currentLine && commentStartIndex <= currentIndex));

                if (withinString)
                {
                    line[currentIndex].mMultiLineComment = inComment;

                    if (c == '\"')
                    {
                        if (currentIndex + 1 < (int)line.size() && line[currentIndex + 1].mChar == '\"')
                        {
                            currentIndex += 1;
                            if (currentIndex < (int)line.size())
                                line[currentIndex].mMultiLineComment = inComment;
                        }
                        else
                            withinString = false;
                    }
                    else if (c == '\\')
                    {
                        currentIndex += 1;
                        if (currentIndex < (int)line.size())
                            line[currentIndex].mMultiLineComment = inComment;
                    }
                }
                else
                {
                    if (c == '\"')
                    {
                        withinString = true;
                        line[currentIndex].mMultiLineComment = inComment;
                    }
                    else
                    {
                        auto pred = [](const char& a, const Glyph& b) { return a == b.mChar; };
                        auto from = line.begin() + currentIndex;
                        auto& startStr = mLanguageDefinition.mCommentStart;
                        auto& singleStartStr = mLanguageDefinition.mSingleLineComment;

                        if (singleStartStr.size() > 0 &&
                            currentIndex + singleStartStr.size() <= line.size() &&
                            equals(singleStartStr.begin(), singleStartStr.end(), from, from + singleStartStr.size(), pred))
                        {
                            withinSingleLineComment = true;
                        }
                        else if (!withinSingleLineComment && currentIndex + startStr.size() <= line.size() &&
                            equals(startStr.begin(), startStr.end(), from, from + startStr.size(), pred))
                        {
                            commentStartLine = currentLine;
                            commentStartIndex = currentIndex;
                        }

                        inComment = inComment = (commentStartLine < currentLine || (commentStartLine == currentLine && commentStartIndex <= currentIndex));

                        line[currentIndex].mMultiLineComment = inComment;
                        line[currentIndex].mComment = withinSingleLineComment;

                        auto& endStr = mLanguageDefinition.mCommentEnd;
                        if (currentIndex + 1 >= (int)endStr.size() &&
                            equals(endStr.begin(), endStr.end(), from + 1 - endStr.size(), from + 1, pred))
                        {
                            commentStartIndex = endIndex;
                            commentStartLine = endLine;
                        }
                    }
                }
                currentIndex += UTF8CharLength(c);
                if (currentIndex >= (int)line.size())
                {
                    currentIndex = 0;
                    ++currentLine;
                }
            }
            else
            {
                currentIndex = 0;
                ++currentLine;
            }
        }
        mCheckComments = false;
    }

    if (mColorRangeMin < mColorRangeMax)
    {
        const int increment = (mLanguageDefinition.mTokenize == nullptr) ? 10 : 10000;
        const int to = std::min(mColorRangeMin + increment, mColorRangeMax);
        ColorizeRange(mColorRangeMin, to);
        mColorRangeMin = to;

        if (mColorRangeMax == mColorRangeMin)
        {
            mColorRangeMin = std::numeric_limits<int>::max();
            mColorRangeMax = 0;
        }
        return;
    }
}

float TextEditor::TextDistanceToLineStart(const Coordinates& aFrom) const
{
    auto& line = mLines[aFrom.mLine];
    float distance = 0.0f;
    float spaceSize = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, " ", nullptr, nullptr).x;
    int colIndex = GetCharacterIndex(aFrom);
    for (size_t it = 0u; it < line.size() && it < colIndex; )
    {
        if (line[it].mChar == '\t')
        {
            distance = (1.0f + std::floor((1.0f + distance) / (4.f * spaceSize))) * (4.f * spaceSize);
            ++it;
        }
        else
        {
            auto d = UTF8CharLength(line[it].mChar);
            char tempCString[7];
            int i = 0;
            for (; i < 6 && d-- > 0 && it < (int)line.size(); i++, it++)
                tempCString[i] = line[it].mChar;

            tempCString[i] = '\0';
            distance += ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, tempCString, nullptr, nullptr).x;
        }
    }

    return distance;
}

TextEditor::UndoRecord::UndoRecord(
    const std::string& aAdded,
    const TextEditor::Coordinates aAddedStart,
    const TextEditor::Coordinates aAddedEnd,
    const std::string& aRemoved,
    const TextEditor::Coordinates aRemovedStart,
    const TextEditor::Coordinates aRemovedEnd,
    TextEditor::EditorState& aBefore,
    TextEditor::EditorState& aAfter)
    : mAdded(aAdded)
    , mAddedStart(aAddedStart)
    , mAddedEnd(aAddedEnd)
    , mRemoved(aRemoved)
    , mRemovedStart(aRemovedStart)
    , mRemovedEnd(aRemovedEnd)
    , mBefore(aBefore)
    , mAfter(aAfter)
{
    assert(mAddedStart <= mAddedEnd);
    assert(mRemovedStart <= mRemovedEnd);
}

void TextEditor::UndoRecord::Undo(TextEditor* aEditor)
{
    if (!mAdded.empty())
    {
        aEditor->DeleteRange(mAddedStart, mAddedEnd);
        aEditor->Colorize(mAddedStart.mLine - 1, mAddedEnd.mLine - mAddedStart.mLine + 2);
    }

    if (!mRemoved.empty())
    {
        auto start = mRemovedStart;
        aEditor->InsertTextAt(start, mRemoved.c_str());
        aEditor->Colorize(mRemovedStart.mLine - 1, mRemovedEnd.mLine - mRemovedStart.mLine + 2);
    }

    aEditor->mState = mBefore;

}

void TextEditor::UndoRecord::Redo(TextEditor* aEditor)
{
    if (!mRemoved.empty())
    {
        aEditor->DeleteRange(mRemovedStart, mRemovedEnd);
        aEditor->Colorize(mRemovedStart.mLine - 1, mRemovedEnd.mLine - mRemovedStart.mLine + 1);
    }

    if (!mAdded.empty())
    {
        auto start = mAddedStart;
        aEditor->InsertTextAt(start, mAdded.c_str());
        aEditor->Colorize(mAddedStart.mLine - 1, mAddedEnd.mLine - mAddedStart.mLine + 1);
    }

    aEditor->mState = mAfter;
}

static bool TokenizeCStyleString(const char* in_begin, const char* in_end, const char*& out_begin, const char*& out_end)
{
    const char* p = in_begin;

    if (*p == '"')
    {
        p++;

        while (p < in_end)
        {
            // handle end of string
            if (*p == '"')
            {
                out_begin = in_begin;
                out_end = p + 1;
                return true;
            }

            // handle escape character for "
            if (*p == '\\' && p + 1 < in_end && p[1] == '"')
                p++;

            p++;
        }
    }

    return false;
}

static bool TokenizeCStyleCharacterLiteral(const char* in_begin, const char* in_end, const char*& out_begin, const char*& out_end)
{
    const char* p = in_begin;

    if (*p == '\'')
    {
        p++;

        // handle escape characters
        if (p < in_end && *p == '\\')
            p++;

        if (p < in_end)
            p++;

        // handle end of character literal
        if (p < in_end && *p == '\'')
        {
            out_begin = in_begin;
            out_end = p + 1;
            return true;
        }
    }

    return false;
}

static bool TokenizeCStyleIdentifier(const char* in_begin, const char* in_end, const char*& out_begin, const char*& out_end)
{
    const char* p = in_begin;

    if ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || *p == '_')
    {
        p++;

        while ((p < in_end) && ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || (*p >= '0' && *p <= '9') || *p == '_'))
            p++;

        out_begin = in_begin;
        out_end = p;
        return true;
    }

    return false;
}

static bool TokenizeCStyleNumber(const char* in_begin, const char* in_end, const char*& out_begin, const char*& out_end)
{
    const char* p = in_begin;

    const bool startsWithNumber = *p >= '0' && *p <= '9';

    if (*p != '+' && *p != '-' && !startsWithNumber)
        return false;

    p++;

    bool hasNumber = startsWithNumber;

    while (p < in_end && (*p >= '0' && *p <= '9'))
    {
        hasNumber = true;

        p++;
    }

    if (hasNumber == false)
        return false;

    bool isFloat = false;
    bool isHex = false;
    bool isBinary = false;

    if (p < in_end)
    {
        if (*p == '.')
        {
            isFloat = true;

            p++;

            while (p < in_end && (*p >= '0' && *p <= '9'))
                p++;
        }
        else if (*p == 'x' || *p == 'X')
        {
            // hex formatted integer of the type 0xef80

            isHex = true;

            p++;

            while (p < in_end && ((*p >= '0' && *p <= '9') || (*p >= 'a' && *p <= 'f') || (*p >= 'A' && *p <= 'F')))
                p++;
        }
        else if (*p == 'b' || *p == 'B')
        {
            // binary formatted integer of the type 0b01011101

            isBinary = true;

            p++;

            while (p < in_end && (*p >= '0' && *p <= '1'))
                p++;
        }
    }

    if (isHex == false && isBinary == false)
    {
        // floating point exponent
        if (p < in_end && (*p == 'e' || *p == 'E'))
        {
            isFloat = true;

            p++;

            if (p < in_end && (*p == '+' || *p == '-'))
                p++;

            bool hasDigits = false;

            while (p < in_end && (*p >= '0' && *p <= '9'))
            {
                hasDigits = true;

                p++;
            }

            if (hasDigits == false)
                return false;
        }

        // single precision floating point type
        if (p < in_end && *p == 'f')
            p++;
    }

    if (isFloat == false)
    {
        // integer size type
        while (p < in_end && (*p == 'u' || *p == 'U' || *p == 'l' || *p == 'L'))
            p++;
    }

    out_begin = in_begin;
    out_end = p;
    return true;
}

static bool TokenizeCStylePunctuation(const char* in_begin, const char* in_end, const char*& out_begin, const char*& out_end)
{
    (void)in_end;

    switch (*in_begin)
    {
    case '[':
    case ']':
    case '{':
    case '}':
    case '!':
    case '%':
    case '^':
    case '&':
    case '*':
    case '(':
    case ')':
    case '-':
    case '+':
    case '=':
    case '~':
    case '|':
    case '<':
    case '>':
    case '?':
    case ':':
    case '/':
    case ';':
    case ',':
    case '.':
        out_begin = in_begin;
        out_end = in_begin + 1;
        return true;
    }

    return false;
}

const TextEditor::LanguageDefinition& TextEditor::LanguageDefinition::CPlusPlus()
{
    static LanguageDefinition langDef;

    static bool inited = false;
    if (!inited)
    {
        langDef.mPurpleKeywords =
        {
             "break", "do", "else", "elseif", "end", "for", "function", "if", "in", "repeat", "return", "then", "until", "while"
        };

        langDef.mBlueKeywords =
        {
            "false", "true", "local", "nil", "not", "or"
        };

        langDef.mKnownFunctions =
        {
            "assert", "collectgarbage", "dofile", "error", "getmetatable", "ipairs", "loadfile", "load", "loadstring",  "next",  "pairs",  "pcall",  "print",  "rawequal",  "rawlen",  "rawget",  "rawset",
            "select",  "setmetatable",  "tonumber",  "tostring",  "type",  "xpcall",  "_G",  "_VERSION","arshift", "band", "bnot", "bor", "bxor", "btest", "extract", "lrotate", "lshift", "replace",
            "rrotate", "rshift", "create", "resume", "running", "status", "wrap", "yield", "isyieldable", "debug","getuservalue", "gethook", "getinfo", "getlocal", "getregistry", "getmetatable",
            "getupvalue", "upvaluejoin", "upvalueid", "setuservalue", "sethook", "setlocal", "setmetatable", "setupvalue", "traceback", "close", "flush", "input", "lines", "open", "output", "popen",
            "read", "tmpfile", "type", "write", "close", "flush", "lines", "read", "seek", "setvbuf", "write", "__gc", "__tostring", "abs", "acos", "asin", "atan", "ceil", "cos", "deg", "exp", "tointeger",
            "floor", "fmod", "ult", "log", "max", "min", "modf", "rad", "random", "randomseed", "sin", "sqrt", "string", "tan", "type", "atan2", "cosh", "sinh", "tanh",
            "pow", "frexp", "ldexp", "log10", "pi", "huge", "maxinteger", "mininteger", "loadlib", "searchpath", "seeall", "preload", "cpath", "path", "searchers", "loaded", "module", "require", "clock",
            "date", "difftime", "execute", "exit", "getenv", "remove", "rename", "setlocale", "time", "tmpname", "byte", "char", "dump", "find", "format", "gmatch", "gsub", "len", "lower", "match", "rep",
            "reverse", "sub", "upper", "pack", "packsize", "unpack", "concat", "maxn", "insert", "pack", "unpack", "remove", "move", "sort", "offset", "codepoint", "char", "len", "codes", "charpattern",
            "coroutine", "table", "io", "os", "string", "utf8", "bit32", "math", "debug", "package"
        };

        langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("L?\\\"(\\\\.|[^\\\"])*\\\"", PaletteIndex::String));
        langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("\\\'[^\\\']*\\\'", PaletteIndex::String));
        langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("0[xX][0-9a-fA-F]+[uU]?[lL]?[lL]?", PaletteIndex::Number));
        langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("[+-]?([0-9]+([.][0-9]*)?|[.][0-9]+)([eE][+-]?[0-9]+)?[fF]?", PaletteIndex::Number));
        langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("[+-]?[0-9]+[Uu]?[lL]?[lL]?", PaletteIndex::Number));
        langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("[a-zA-Z_][a-zA-Z0-9_]*", PaletteIndex::Identifier));
        langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("[\\[\\]\\{\\}\\!\\%\\^\\&\\*\\(\\)\\-\\+\\=\\~\\|\\<\\>\\?\\/\\;\\,\\.]", PaletteIndex::Punctuation));

        langDef.mCommentStart = "--[[";
        langDef.mCommentEnd = "]]";
        langDef.mSingleLineComment = "--";
        langDef.mCaseSensitive = true;
        langDef.mAutoIndentation = false;

        inited = true;
    }

    return langDef;
}