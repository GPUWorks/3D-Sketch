
#include "stdafx.h"
#include "cmdview.h"
#include "3dsketch.h"
#include "3dview.h"


using namespace graphic;

cCmdView::cCmdView(const string &name) : framework::cDockWindow(name) 
{
}

cCmdView::~cCmdView() 
{
}


bool cCmdView::Init(graphic::cRenderer &renderer) 
{
	Read("command.txt");
	return true;
}


void cCmdView::OnRender(const float deltaSeconds)
{
	ImGui::Text("Command");
	
	ImGui::SameLine(0, 20);
	if (ImGui::Button(" Process "))
	{
		Parse();
	}

	ImGui::SameLine(0, 100);
	if (ImGui::Button("?", ImVec2(50,0)))
	{
		cViewer *viewer = (cViewer*)g_application;
		c3DView *p3dView = viewer->m_3dView;
		p3dView->m_showHelp = !p3dView->m_showHelp;
	}

	const float w = m_rect.Width() - 20;
	const float h = m_rect.Height() - 70;
	ImGui::InputTextMultiline(" ", m_text.m_str, sizeof(m_text.m_str)
		, ImVec2(w, h));
}


// ��ɾ� �м�
bool cCmdView::Parse()
{
	m_cmds.clear();
	m_vars.clear();

	const char *str = m_text.m_str;
	while (1)
	{
		Str256 line;
		str = GetLine(str, line);
		ParseLine(line.m_str);

		if (*str == NULL)
			break; // Line End
	}

	// ��ɾ� �м� ó���� ���ÿ� ���Ͽ� �����Ѵ�.
	Write("command.txt");
	return true;
}


// store out, line string 
// return next string
const char* cCmdView::GetLine(const char *str, OUT Str256 &out)
{
	const int maxSize = sizeof(out.m_str) - 1;
	int i = 0;
	while (*str && (i < maxSize) && (*str != 10)) // Line feed (LF)
	{
		char c = *str++;
		out.m_str[i++] = c;
	}
	out.m_str[i] = NULL;

	if (*str == 10)
	{
		++str; // 10 (LF)
	}

	return str;
}


//
// Vector Type
//		[+/-] string [{] x '=' number y '=' number z '=' number [}] string
// float Type
//		string number string
// Triangle tri1, curPos, curPos, curPos
// Direction dir1, curPos, dir
// Collision tri1, dir1
// Box box1, curPos, 0.1
bool cCmdView::ParseLine(const char *str)
{
	if ((*str == '+') || (*str == '-'))
		++str;

	StrId id;
	str = Str(str, id);
	if (id.empty())
		return false; // error

	const sCmd::Enum funcType = GetFunctionType(id);

	if (sCmd::NONE != funcType)
	{ 
		ParseFunction(funcType, str);
	}
	else if (*str == '{')
	{
		str = Match(str, '{');

		StrId var1; //x
		str = Str(str, var1);
		str = Match(str, '=');
		StrId num1;
		str = Number(str, num1);

		StrId var2; //y
		str = Str(str, var2);
		str = Match(str, '=');
		StrId num2;
		str = Number(str, num2);

		StrId var3; //z
		str = Str(str, var3);
		str = Match(str, '=');
		StrId num3;
		str = Number(str, num3);

		StrId type;
		str = Str(str, type);

		str = Match(str, '}');

		sSymbol symbol;
		symbol.type = type;
		symbol.val1 = Vector3((float)atof(num1.m_str), (float)atof(num2.m_str), (float)atof(num3.m_str));
		m_vars[id] = symbol;
	}
	else
	{
		StrId num;
		str = Number(str, num);
		
		StrId type;
		str = Str(str, type);

		sSymbol symbol;
		symbol.type = type;
		symbol.val1 = Vector3(1,1,1) * (float)atof(num.m_str);
		m_vars[id] = symbol;
	}

	return true;
}


// Triangle tri1, curPos, curPos, curPos
// Direction dir1, curPos, dir
// Box box1, curPos, 0.1
// Collision tri1, dir1
bool cCmdView::ParseFunction(const sCmd::Enum func, const char *str)
{
	switch (func)
	{
	case sCmd::TRIANGLE:
	{
		sCmd cmd;
		cmd.cmd = func;
		str = Str(str, cmd.id); // Triangle Variable Id
		str = Match(str, ',');
		str = Str(str, cmd.arg1); // vertex pos 1
		str = Match(str, ',');
		str = Str(str, cmd.arg2); // vertex pos 2
		str = Match(str, ',');
		str = Str(str, cmd.arg3); // vertex pos 3
		m_cmds.push_back(cmd);
	}
	break;

	case sCmd::DIRECTION:
	{
		sCmd cmd;
		cmd.cmd = func;
		str = Str(str, cmd.id); // Direction Variable Id
		str = Match(str, ',');
		str = Str(str, cmd.arg1); // Origin Pos
		str = Match(str, ',');
		str = Str(str, cmd.arg2); // Direction Vector
		m_cmds.push_back(cmd);
	}
	break;

	case sCmd::BOX:
	{
		sCmd cmd;
		cmd.cmd = func;
		str = Str(str, cmd.id); // Box Variable Id
		str = Match(str, ',');
		str = Str(str, cmd.arg1); // Box Pos
		str = Match(str, ',');
		if (IsNumber(str)) // Box Size
			str = Number(str, cmd.arg2);
		else
			str = Str(str, cmd.arg2);
		m_cmds.push_back(cmd);
	}
	break;

	case sCmd::COLLISION:
	{
		sCmd cmd;
		cmd.cmd = func;
		str = Str(str, cmd.arg1); // Triangle
		str = Match(str, ',');
		str = Str(str, cmd.arg2); // Direction
		m_cmds.push_back(cmd);
	}
	break;

	default:
		assert(0);
		break;
	}

	return true;
}


// return string (alphabet) + number
const char* cCmdView::Str(const char *str, OUT StrId &out)
{
	const int maxSize = sizeof(out.m_str) - 1;
	int i = 0;
	while (*str && (i < maxSize) 
		&& (is_alpha(*str) || isdigit(*str) 
			|| ((i == 0) && (*str == ' ')))) // alphabet
	{
		char c = *str++;
		if (c != ' ') // ignore space character
			out.m_str[i++] = c;
	}
	out.m_str[i] = NULL;

	return str;
}


// return number, float, int
const char* cCmdView::Number(const char *str, OUT StrId &out)
{
	const char *digitOp = ".+-e";
	const int maxSize = sizeof(out.m_str) - 1;
	int i = 0;
	while (*str && (i < maxSize) 
		&& (isdigit(*str) || strchr(digitOp, *str)
			|| ((i == 0) && (*str == ' ')))) // number ex) -11.6843023
	{
		char c = *str++;
		out.m_str[i++] = c;
	}
	out.m_str[i] = NULL;

	return str;
}


// test match, if not match, return null
const char* cCmdView::Match(const char *str, const char c)
{
	if (*str == c)
		return ++str;
	else
		return str;
}


// str�� ���ڸ� true�� �����Ѵ�.
bool cCmdView::IsNumber(const char *str)
{
	const char *p = str;
	while (*p)
	{
		if (*p == ' ')
		{
			++p;
			continue;
		}

		if (isalpha(*p))
			return false;
		else
			return true;
	}
	return false;
}


cCmdView::sCmd::Enum cCmdView::GetFunctionType(const StrId &str)
{
	if (str == "Triangle")
	{
		return sCmd::TRIANGLE;
	}
	else if (str == "Box")
	{
		return sCmd::BOX;
	}
	else if (str == "Direction")
	{
		return sCmd::DIRECTION;
	}
	else if (str == "Collision")
	{
		return sCmd::COLLISION;
	}

	return sCmd::NONE;
}


bool cCmdView::Read(const char *fileName)
{
	std::ifstream ifs(fileName);
	if (!ifs.is_open())
		return false;

	ifs.read(m_text.m_str, sizeof(m_text.m_str));

	return true;
}


bool cCmdView::Write(const char *fileName)
{
	std::ofstream ofs(fileName);
	if (!ofs.is_open())
		return false;

	ofs << m_text.m_str;

	return true;
}
