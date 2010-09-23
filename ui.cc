/*
 * ui.cc
 *
 *  Created on: 2009-9-16
 *      Author: thor
 */

// 国际化支持
#include <glib/gi18n.h>
#include <locale.h>

#define PACKAGE "cnchess"
#define LOCALEDIR "/usr/share/cnchess/locale"

#include "engine.h"
#include <gtkmm.h>
#if defined(__gnu_linux__)
#include <linux/limits.h>
#endif
#include <gdk/gdkkeysyms.h>
#include <gtkmm/accelmap.h>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <tlib/tlib.h>
#include <librsvg/rsvg.h>
#include <librsvg/rsvg-cairo.h>

extern const unsigned char ui_data[];
extern const unsigned char svg_data[];
extern const unsigned int svg_data_length;
// 程序图标
extern const guint8 icon_inline[];

using namespace std;

static const char* piece_code[2][7] = {
{ "#shuai", "#red_shi", "#red_xiang", "#ma", "#che", "#pao", "#bing" },
{ "#jiang", "#black_shi", "#black_xiang", "#ma", "#che", "#pao", "#zu" } };


// 使用 Gdk 函数绘制棋盘网格
#define GRID_USE_GDK

class Chess
{
public:
	inline Chess(int argc, char **argv);
	inline ~Chess();
	inline void run();
	void init(uint8 player);
private:
	inline void compute_cell_size();
	// 电脑走棋
	inline void sync();
	// 创建一个线程让电脑走棋
	inline void response();
	inline void msgbox(const char* msg);
	// 真正电脑走棋
	void response_move(void);
	void on_new_red();
	void on_new_black();
	void on_back();
	void on_quit();
	void on_ai_easy();
	void on_ai_normal();
	void on_ai_hard();
	void on_size_small();
	void on_size_normal();
	void on_size_big();
	void on_setting_voice();
	void on_setting_music();
	void on_about();
	bool on_drawarea_expose_event(GdkEventExpose *event);
	bool on_drawarea_button_press_event(GdkEventButton *event);
	void draw_svg_img(Cairo::RefPtr<Cairo::Context> cr, \
			const char* id, int left, int top, double scale_rate);
	void draw_chessman(Cairo::RefPtr<Cairo::Context> cr,
			int x, int y, int imgx, int imgy);
#ifdef GRID_USE_GDK
	void draw_cross(Glib::RefPtr<Gdk::Window>& canvas,
		Glib::RefPtr<Gdk::GC>& gc, int x, int y, int type);
#else
	void draw_cross(Cairo::RefPtr<Cairo::Context> cr, int x, int y, int type);
#endif
	inline void refresh();
	inline void playsound(const char* filename);
private:
	Glib::RefPtr<Gtk::Window> _window;
	Glib::RefPtr<Gtk::DrawingArea> _drawarea;
	Glib::RefPtr<Gtk::Statusbar> _statusbar;
	int _ai_level;
	int _board_size;
	int _cell_size;
	int _chessman_size;
	bool _voice;
	bool _music;
	string _setting_file;
	EnginePtr _engine;
	uint8 _player;
	uint8 _board_clone[256];
	RsvgHandle *_svg;
	uint32 _select;
	uint32 _last_move;
	bool _game_over;
	bool _computing;
	string _exe_path;
	Cairo::RefPtr<Cairo::ImageSurface> _imgbuf;
};

inline Chess::Chess(int argc, char** argv)
{
	// 得到应用程序所在路径
#if defined(__gnu_linux__)
	if (argv[0][0] != '/')
	{
		char buffer[PATH_MAX];
		_exe_path = getcwd(buffer, PATH_MAX);
		_exe_path += "/";
		_exe_path += argv[0];
	}
	else
	{
		_exe_path = argv[0];
	}
	tlib::replace<char>(_exe_path, "/./", "/");
	_exe_path = _exe_path.substr(0, _exe_path.rfind('/'));
#else
#error "not implement..."
#endif

	// 创建引擎
	_engine = Engine::create();
	// 加载 SVG 资源
	GError *err;
	_svg = rsvg_handle_new_from_data(svg_data, svg_data_length, &err);
	// 加载配置文件
#if defined(__gnu_linux__)
	const char* home = getenv("HOME");
#elif defined(__WIN32__)
	const char* home = getenv("USERPROFILE");
#else
#error "not implement..."
#endif
	_board_size = 2;
	_ai_level = 2;
	_voice = true;
	_music = true;
	if (home)
	{
		_setting_file = home;
		_setting_file += "/.chess2";
		map<string, string> setting;
		tlib::load_setting(_setting_file, setting);
		if (setting.find("size") != setting.end())
		{
			_board_size = tlib::strto<int>(setting["size"]);
			if (_board_size < 1)
				_board_size = 1;
			else if (_board_size > 3)
				_board_size = 3;
		}
		if (setting.find("level") != setting.end())
		{
			_ai_level = tlib::strto<int>(setting["level"]);
			if (_ai_level < 1)
				_ai_level = 1;
			else if (_ai_level > 3)
				_ai_level = 3;
		}
		if (setting.find("voice") != setting.end())
			_voice = tlib::strto<bool>(setting["voice"]);
		if (setting.find("music") != setting.end())
			_music = tlib::strto<bool>(setting["music"]);
	}

	// 构造界面
	Glib::RefPtr<Gtk::Builder> builder
		= Gtk::Builder::create_from_string((const char *)ui_data);
	_window = Glib::RefPtr<Gtk::Window>::cast_dynamic(
			builder->get_object("win_main"));
	_drawarea = Glib::RefPtr<Gtk::DrawingArea>::cast_dynamic(
			builder->get_object("board_area"));
	_statusbar = Glib::RefPtr<Gtk::Statusbar>::cast_dynamic(
			builder->get_object("statusbar"));


	Glib::RefPtr<Gtk::MenuItem> item = Glib::RefPtr<Gtk::MenuItem>::cast_dynamic(
			builder->get_object("menu_new_red"));
	item->signal_activate().connect(sigc::mem_fun(*this, &Chess::on_new_red));
	item = Glib::RefPtr<Gtk::MenuItem>::cast_dynamic(
			builder->get_object("menu_new_black"));
	item->signal_activate().connect(sigc::mem_fun(*this, &Chess::on_new_black));
	item = Glib::RefPtr<Gtk::MenuItem>::cast_dynamic(
			builder->get_object("menu_back"));
	item->signal_activate().connect(sigc::mem_fun(*this, &Chess::on_back));
	item = Glib::RefPtr<Gtk::MenuItem>::cast_dynamic(
			builder->get_object("menu_quit"));
	item->signal_activate().connect(sigc::mem_fun(*this, &Chess::on_quit));
	item = Glib::RefPtr<Gtk::MenuItem>::cast_dynamic(
			builder->get_object("menu_ai_easy"));
	if (_ai_level == 1)
	{
		item->activate();
		_statusbar->push(_("Difficulty Level: Easy"));
	}
	item->signal_activate().connect(sigc::mem_fun(*this, &Chess::on_ai_easy));

	item = Glib::RefPtr<Gtk::MenuItem>::cast_dynamic(
			builder->get_object("menu_ai_normal"));
	if (_ai_level == 2)
	{
		item->activate();
		_statusbar->push(_("Difficulty Level: Normal"));
	}
	item->signal_activate().connect(sigc::mem_fun(*this, &Chess::on_ai_normal));

	item = Glib::RefPtr<Gtk::MenuItem>::cast_dynamic(
			builder->get_object("menu_ai_hard"));
	if (_ai_level == 3)
	{
		item->activate();
		_statusbar->push(_("Difficulty Level: Hard"));
	}
	item->signal_activate().connect(sigc::mem_fun(*this, &Chess::on_ai_hard));

	item = Glib::RefPtr<Gtk::MenuItem>::cast_dynamic(
			builder->get_object("menu_size_small"));
	if (_board_size == 1)
		item->activate();
	item->signal_activate().connect(sigc::mem_fun(*this, &Chess::on_size_small));

	item = Glib::RefPtr<Gtk::MenuItem>::cast_dynamic(
			builder->get_object("menu_size_normal"));
	if (_board_size == 2)
		item->activate();
	item->signal_activate().connect(sigc::mem_fun(*this, &Chess::on_size_normal));

	item = Glib::RefPtr<Gtk::MenuItem>::cast_dynamic(
			builder->get_object("menu_size_big"));
	if (_board_size == 3)
		item->activate();
	item->signal_activate().connect(sigc::mem_fun(*this, &Chess::on_size_big));

	Glib::RefPtr<Gtk::CheckMenuItem> ckitem;
	ckitem = Glib::RefPtr<Gtk::CheckMenuItem>::cast_dynamic(
			builder->get_object("menu_voice"));
	ckitem->set_active(_voice);
	ckitem->signal_activate().connect(sigc::mem_fun(*this, &Chess::on_setting_voice));

	ckitem = Glib::RefPtr<Gtk::CheckMenuItem>::cast_dynamic(
			builder->get_object("menu_music"));
	ckitem->set_active(_music);
	ckitem->signal_activate().connect(sigc::mem_fun(*this, &Chess::on_setting_music));

	item = Glib::RefPtr<Gtk::MenuItem>::cast_dynamic(
			builder->get_object("menu_about"));
	item->signal_activate().connect(sigc::mem_fun(*this, &Chess::on_about));

	// 计算出方格的大小
	compute_cell_size();

	_drawarea->set_events(Gdk::BUTTON_PRESS_MASK | Gdk::KEY_PRESS_MASK);

	// 连接画布的绘制事件
	_drawarea->signal_expose_event().connect(
			sigc::mem_fun(*this, &Chess::on_drawarea_expose_event));

	// 连接画布的鼠标按下事件
	_drawarea->signal_button_press_event().connect(
			sigc::mem_fun(*this, &Chess::on_drawarea_button_press_event));

	// 设置窗口总是居中
	_window->set_position(Gtk::WIN_POS_CENTER_ALWAYS);
	_window->set_title(_("Chinese Chess"));

	// 设置图标
	Glib::RefPtr<Gdk::Pixbuf> icon =
			Gdk::Pixbuf::create_from_inline(-1, icon_inline, false);
	_window->set_icon(icon);


	init(0);
}

void Chess::init(uint8 player)
{
	_engine->init();
	_player = player;
	sync();
	_select = 0;
	_last_move = 0;
	_game_over = false;
	_computing = false;
}

inline void Chess::sync()
{
	memcpy(_board_clone, _engine->_board._board, 256);
}

inline Chess::~Chess()
{
	map<string, string> setting;
	setting["size"] = tlib::strfrom<int>(_board_size);
	setting["level"] = tlib::strfrom<int>(_ai_level);
	setting["voice"] = tlib::strfrom<bool>(_voice);
	setting["music"] = tlib::strfrom<bool>(_music);

	try
	{
		tlib::save_setting(_setting_file, setting);
	}
	catch(...)
	{
	}
	GError *err;
	rsvg_handle_close(_svg, &err);
}

void Chess::draw_svg_img(Cairo::RefPtr<Cairo::Context> cr, \
		const char* id, int left, int top, double scale_rate)
{
	RsvgPositionData pos_data;
	rsvg_handle_get_position_sub(_svg, &pos_data, id);
	RsvgDimensionData size_data;
	rsvg_handle_get_dimensions_sub(_svg, &size_data, id);

	cr->save();
	cr->scale(scale_rate, scale_rate);
	double l = (left) / scale_rate;
	double t = (top ) / scale_rate;
	cr->translate(l - pos_data.x - size_data.width / 2, t - pos_data.y - size_data.height / 2);
	rsvg_handle_render_cairo_sub(_svg, cr->cobj(), id);
	cr->restore();
}


// 根据屏幕大小，计算出棋子格子的大小
inline void Chess::compute_cell_size()
{
	int screen_height = Gdk::screen_height();
	switch (_board_size)
	{
	case 1:
		_cell_size = (double)screen_height / 30;
		break;
	case 2:
		_cell_size = (double)screen_height / 2 / 10;
		break;
	case 3:
		_cell_size = (double)screen_height / 10 * 7 / 10;
		break;
	default:
		_cell_size = (double)screen_height / 30;
	}
	// 设置画布的最小尺寸
	_drawarea->set_size_request(_cell_size * 8 + _cell_size * 2,
			_cell_size * 9 + _cell_size * 2);

	// 在内存中绘制出棋子图片，避免今后每次绘图都直接绘制SVG图片
	_chessman_size = _cell_size * 0.9;
	_imgbuf = Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32,
			_chessman_size * 7, _chessman_size * 2);
	Cairo::RefPtr<Cairo::Context> cr = Cairo::Context::create(_imgbuf);

	// 使用红子背景决定缩放比例
	RsvgDimensionData size_data;
	rsvg_handle_get_dimensions_sub(_svg, &size_data, "#bk_red");
	double scale_rate = _chessman_size / (double)size_data.width;

	for (int y = 0; y < 2; y++)
	{
		for (int x = 0; x < 7; x++)
		{
			if (y == 0)
				draw_svg_img(cr, "#bk_red",
						(x + 0.5) * _chessman_size, (y + 0.5) * _chessman_size, scale_rate);
			else
				draw_svg_img(cr, "#bk_black",
						(x + 0.5) * _chessman_size, (y + 0.5) * _chessman_size, scale_rate);

			draw_svg_img(cr, piece_code[y][x],
					(x + 0.5) * _chessman_size, (y + 0.5) * _chessman_size, scale_rate);
		}
	}

}

inline void Chess::run()
{
	gdk_threads_enter();
	Gtk::Main::run(*_window.operator->());
	gdk_threads_leave();
}

void Chess::on_new_red()
{
	if (_computing)
		return;
	init(0);
	refresh();
}

void Chess::on_new_black()
{
	if (_computing)
		return;
	init(1);
	refresh();
	sleep(2);
	response();
}

void Chess::on_back()
{
	if (_engine->_board._player != _player)
		return;
	if (_computing)
		return;
	if (_engine->_board._records.size() >= 2)
	{
		_engine->_board.undo_move();
		_engine->_board.undo_move();
		_select = 0;
		_game_over = false;
		sync();
		refresh();
	}
}

void Chess::on_quit()
{
	Gtk::Main::quit();
}

void Chess::on_ai_easy()
{
	if (_ai_level != 1)
	{
		_ai_level = 1;
		_statusbar->pop();
		_statusbar->push(_("Difficulty Level: Easy"));
	}
}

void Chess::on_ai_normal()
{
	if (_ai_level != 2)
	{
		_ai_level = 2;
		_statusbar->pop();
		_statusbar->push(_("Difficulty Level: Normal"));
	}
}

void Chess::on_ai_hard()
{
	if (_ai_level != 3)
	{
		_ai_level = 3;
		_statusbar->pop();
		_statusbar->push(_("Difficulty Level: Hard"));
	}
}

void Chess::on_size_small()
{
	if (_board_size != 1)
	{
		_board_size = 1;
		compute_cell_size();
	}
}

void Chess::on_size_normal()
{
	if (_board_size != 2)
	{
		_board_size = 2;
		compute_cell_size();
	}
}

void Chess::on_size_big()
{
	if (_board_size != 3)
	{
		_board_size = 3;
		compute_cell_size();
	}
}

void Chess::on_setting_voice()
{
	_voice = !_voice;
}

void Chess::on_setting_music()
{
	_music = !_music;
}

void Chess::on_about()
{
	Gtk::AboutDialog about;
	about.set_name(_("Chinese Chess"));
	std::vector<string> authors;
	authors.push_back(_("Thor Qin <thor.qin@gmail.com>"));

	about.set_authors(authors);
	about.set_copyright("Copyright © 2009 Thor Qin");
	about.set_version("1.0");
	about.run();
}

#ifdef GRID_USE_GDK
void Chess::draw_cross(Glib::RefPtr<Gdk::Window>& canvas,
		Glib::RefPtr<Gdk::GC>& gc, int x, int y, int type)
{
	int line_len = _cell_size / 6;
	int space = 2 + _cell_size / 20;
	if (type & 1)
	{

		canvas->draw_line(gc, _cell_size + _cell_size * x - space - line_len,
				_cell_size + _cell_size * y - space,
				_cell_size + _cell_size * x - space,
				_cell_size + _cell_size * y - space);
		canvas->draw_line(gc, _cell_size + _cell_size * x - space,
				_cell_size + _cell_size * y - space,
				_cell_size + _cell_size * x - space,
				_cell_size + _cell_size * y - space - line_len);

		canvas->draw_line(gc, _cell_size + _cell_size * x - space - line_len,
				_cell_size + _cell_size * y + space,
				_cell_size + _cell_size * x - space,
				_cell_size + _cell_size * y + space);
		canvas->draw_line(gc, _cell_size + _cell_size * x - space,
				_cell_size + _cell_size * y + space,
				_cell_size + _cell_size * x - space,
				_cell_size + _cell_size * y + space + line_len);

//		cr->move_to(_cell_size + _cell_size * x - space - line_len,
//				_cell_size + _cell_size * y - space);
//		cr->line_to(_cell_size + _cell_size * x - space,
//				_cell_size + _cell_size * y - space);
//		cr->line_to(_cell_size + _cell_size * x - space,
//				_cell_size + _cell_size * y - space - line_len);
//
//		cr->move_to(_cell_size + _cell_size * x - space - line_len,
//				_cell_size + _cell_size * y + space);
//		cr->line_to(_cell_size + _cell_size * x - space,
//				_cell_size + _cell_size * y + space);
//		cr->line_to(_cell_size + _cell_size * x - space,
//				_cell_size + _cell_size * y + space + line_len);
	}
	if (type & 2)
	{
		canvas->draw_line(gc, _cell_size + _cell_size * x + space + line_len,
				_cell_size + _cell_size * y - space,
				_cell_size + _cell_size * x + space,
				_cell_size + _cell_size * y - space);
		canvas->draw_line(gc, _cell_size + _cell_size * x + space,
				_cell_size + _cell_size * y - space,
				_cell_size + _cell_size * x + space,
				_cell_size + _cell_size * y - space - line_len);

		canvas->draw_line(gc, _cell_size + _cell_size * x + space + line_len,
				_cell_size + _cell_size * y + space,
				_cell_size + _cell_size * x + space,
				_cell_size + _cell_size * y + space);
		canvas->draw_line(gc, _cell_size + _cell_size * x + space,
				_cell_size + _cell_size * y + space,
				_cell_size + _cell_size * x + space,
				_cell_size + _cell_size * y + space + line_len);

//		cr->move_to(_cell_size + _cell_size * x + space + line_len,
//				_cell_size + _cell_size * y - space);
//		cr->line_to(_cell_size + _cell_size * x + space,
//				_cell_size + _cell_size * y - space);
//		cr->line_to(_cell_size + _cell_size * x + space,
//				_cell_size + _cell_size * y - space - line_len);
//
//		cr->move_to(_cell_size + _cell_size * x + space + line_len,
//				_cell_size + _cell_size * y + space);
//		cr->line_to(_cell_size + _cell_size * x + space,
//				_cell_size + _cell_size * y + space);
//		cr->line_to(_cell_size + _cell_size * x + space,
//				_cell_size + _cell_size * y + space + line_len);
	}
}
#else
void Chess::draw_cross(Cairo::RefPtr<Cairo::Context> cr, int x, int y, int type)
{
	int line_len = _cell_size / 6;
	int space = 2 + _cell_size / 20;
	if (type & 1)
	{
		cr->move_to(_cell_size + _cell_size * x - space - line_len,
				_cell_size + _cell_size * y - space);
		cr->line_to(_cell_size + _cell_size * x - space,
				_cell_size + _cell_size * y - space);
		cr->line_to(_cell_size + _cell_size * x - space,
				_cell_size + _cell_size * y - space - line_len);

		cr->move_to(_cell_size + _cell_size * x - space - line_len,
				_cell_size + _cell_size * y + space);
		cr->line_to(_cell_size + _cell_size * x - space,
				_cell_size + _cell_size * y + space);
		cr->line_to(_cell_size + _cell_size * x - space,
				_cell_size + _cell_size * y + space + line_len);
	}
	if (type & 2)
	{
		cr->move_to(_cell_size + _cell_size * x + space + line_len,
				_cell_size + _cell_size * y - space);
		cr->line_to(_cell_size + _cell_size * x + space,
				_cell_size + _cell_size * y - space);
		cr->line_to(_cell_size + _cell_size * x + space,
				_cell_size + _cell_size * y - space - line_len);

		cr->move_to(_cell_size + _cell_size * x + space + line_len,
				_cell_size + _cell_size * y + space);
		cr->line_to(_cell_size + _cell_size * x + space,
				_cell_size + _cell_size * y + space);
		cr->line_to(_cell_size + _cell_size * x + space,
				_cell_size + _cell_size * y + space + line_len);
	}
}
#endif

void Chess::draw_chessman(Cairo::RefPtr<Cairo::Context> cr,
		int x, int y, int imgx, int imgy)
{
	cr->save();
	int offx = imgx * _chessman_size;
	int offy = imgy * _chessman_size;

	int drawx = _cell_size * x - _chessman_size / 2.0;
	int drawy = _cell_size * y - _chessman_size / 2.0;

	cr->translate(drawx, drawy);

	cr->set_source(_imgbuf, -offx, -offy);
	cr->rectangle(0, 0, _chessman_size, _chessman_size);
	cr->clip();
	cr->paint();


	cr->restore();
}

bool Chess::on_drawarea_expose_event(GdkEventExpose *event)
{
	Cairo::RefPtr<Cairo::Context> cr =
			_drawarea->get_window()->create_cairo_context();

	// 填充底色
	cr->rectangle(0, 0,	_cell_size * 10, _cell_size * 11);
	cr->set_source_rgb(0.6, 0.6, 0.6);
	cr->fill();
	// 填充棋盘底色
	int side = _cell_size / 10;
	cr->rectangle(side, side,	_cell_size * 10 - side * 2, _cell_size * 11 - side * 2);
	cr->set_source_rgb(1, 1, 1);
	cr->fill();


#ifdef GRID_USE_GDK
	// 使用Gdk绘制棋盘线
	Glib::RefPtr<Gdk::Window> canvas = _drawarea->get_window();
	Glib::RefPtr<Gdk::GC> gc = Gdk::GC::create(canvas);
	Gdk::Color color("#b0b0b0");
	//color.set_rgb(200, 100, 100);
	gc->set_rgb_fg_color(color);

	// 画棋盘线
	canvas->draw_rectangle(gc, false, _cell_size, _cell_size, _cell_size * 8, _cell_size * 9);
	// 画横线
	for (int i = 0; i < 8; i++)
	{
		canvas->draw_line(gc, _cell_size, _cell_size * 2 + _cell_size * i,
				_cell_size + _cell_size * 8, _cell_size * 2 + _cell_size * i);
	}
	// 画竖线
	for (int i = 0; i < 7; i++)
	{
		canvas->draw_line(gc, _cell_size * 2 + _cell_size * i, _cell_size,
				_cell_size * 2 + _cell_size * i, _cell_size * 5);

		canvas->draw_line(gc, _cell_size * 2 + _cell_size * i, _cell_size * 6,
				_cell_size * 2 + _cell_size * i, _cell_size * 10);

		// 画棋子摆放点
		if (i == 0)
		{
			draw_cross(canvas, gc, i, 3, 2);
			draw_cross(canvas, gc, i, 6, 2);
			draw_cross(canvas, gc, i + 1, 2, 3);
			draw_cross(canvas, gc, i + 1, 7, 3);
		}
		else if (i == 6)
		{
			draw_cross(canvas, gc, i + 2, 3, 1);
			draw_cross(canvas, gc, i + 2, 6, 1);
			draw_cross(canvas, gc, i + 1, 2, 3);
			draw_cross(canvas, gc, i + 1, 7, 3);
		}
		else if (i == 1 || i == 3 || i == 5)
		{
			draw_cross(canvas, gc, i + 1, 3, 3);
			draw_cross(canvas, gc, i + 1, 6, 3);
		}
	}

	// 画九宫交叉线
	canvas->draw_line(gc, _cell_size * 4, _cell_size,
			_cell_size * 6, _cell_size * 3);
	canvas->draw_line(gc, _cell_size * 4, _cell_size * 8,
			_cell_size * 6, _cell_size * 10);
	canvas->draw_line(gc, _cell_size * 6, _cell_size,
			_cell_size * 4, _cell_size * 3);
	canvas->draw_line(gc, _cell_size * 6, _cell_size * 8,
			_cell_size * 4, _cell_size * 10);


#else
	// 画棋盘线
	cr->rectangle(_cell_size, _cell_size, _cell_size * 8, _cell_size * 9);
	// 画横线
	for (int i = 0; i < 8; i++)
	{
		cr->move_to(_cell_size, _cell_size * 2 + _cell_size * i);
		cr->line_to(_cell_size + _cell_size * 8,
				_cell_size * 2 + _cell_size * i);
	}
	// 画竖线
	for (int i = 0; i < 7; i++)
	{
		cr->move_to(_cell_size * 2 + _cell_size * i, _cell_size);
		cr->line_to(_cell_size * 2 + _cell_size * i, _cell_size * 5);

		cr->move_to(_cell_size * 2 + _cell_size * i, _cell_size * 6);
		cr->line_to(_cell_size * 2 + _cell_size * i, _cell_size * 10);
		// 画棋子摆放点
		if (i == 0)
		{
			draw_cross(cr, i, 3, 2);
			draw_cross(cr, i, 6, 2);
			draw_cross(cr, i + 1, 2, 3);
			draw_cross(cr, i + 1, 7, 3);
		}
		else if (i == 6)
		{
			draw_cross(cr, i + 2, 3, 1);
			draw_cross(cr, i + 2, 6, 1);
			draw_cross(cr, i + 1, 2, 3);
			draw_cross(cr, i + 1, 7, 3);
		}
		else if (i == 1 || i == 3 || i == 5)
		{
			draw_cross(cr, i + 1, 3, 3);
			draw_cross(cr, i + 1, 6, 3);
		}
	}
	// 画九宫交叉线
	cr->move_to(_cell_size * 4, _cell_size);
	cr->line_to(_cell_size * 6, _cell_size * 3);
	cr->move_to(_cell_size * 4, _cell_size * 8);
	cr->line_to(_cell_size * 6, _cell_size * 10);
	cr->move_to(_cell_size * 6, _cell_size);
	cr->line_to(_cell_size * 4, _cell_size * 3);
	cr->move_to(_cell_size * 6, _cell_size * 8);
	cr->line_to(_cell_size * 4, _cell_size * 10);

	// 描线
	cr->set_antialias(Cairo::ANTIALIAS_NONE);
//	double pxw = 1, pxh = 1;
//	cr->device_to_user_distance(pxw, pxh);
//	double px = max<double>(pxw, pxh);
	cr->set_line_width(1);
	cr->set_source_rgb(0.5, 0.5, 0.5);
	cr->stroke();
#endif

	cr->save();
	// 画最后移动的路线
	if (_engine->_board._records.size() > 0)
	{
		uint32 mv = _engine->_board._records.back().mv;
		int x0, y0, x1, y1;
		if (_player == 1) // 游戏者使用黑色
		{
			x0 = flip_column(column(from(mv))) - 2;
			y0 = flip_row(row(from(mv))) - 2;
			x1 = flip_column(column(to(mv))) - 2;
			y1 = flip_row(row(to(mv))) - 2;
		}
		else
		{
			x0 = column(from(mv)) - 2;
			y0 = row(from(mv)) - 2;
			x1 = column(to(mv)) - 2;
			y1 = row(to(mv)) - 2;
		}
		cr->move_to(_cell_size * x0, _cell_size * y0);
		cr->line_to(_cell_size * x1, _cell_size * y1);
		cr->set_source_rgba(0.2, 0.2, 0.6, 0.8);
		cr->set_line_width(6);
		vector<double> vd;
		vd.push_back(7);
		vd.push_back(1);
		cr->set_dash(vd, 0);
		//cr->set_line_cap(Cairo::LINE_CAP_ROUND);
		cr->stroke();
	}
	cr->restore();

	// 画棋子
	for (int i = 0x33; i <= 0xcb; i++)
	{
		if (_board_clone[i] == 0)
			continue;

		int x, y;
		if (_player == 1) // 游戏者使用黑色
		{
			x = flip_column(column(i)) - 2;
			y = flip_row(row(i)) - 2;
		}
		else
		{
			x = column(i) - 2;
			y = row(i) - 2;
		}
		draw_chessman(cr, x, y, _board_clone[i] & 7, _board_clone[i] >= 16);
	}

	// 画选择框
	if (_select)
	{
		int x, y;
		if (_player == 1) // 游戏者使用黑色
		{
			x = flip_column(column(_select)) - 2;
			y = flip_row(row(_select)) - 2;
		}
		else
		{
			x = column(_select) - 2;
			y = row(_select) - 2;
		}

		cr->rectangle((x - 0.5) * _cell_size,
				(y - 0.5) * _cell_size,
				_cell_size,
				_cell_size);
		cr->set_line_width(_cell_size / 20);
		cr->set_source_rgb(0.5, 0.5, 1);
		cr->stroke();
	}


	return true;
}

inline void Chess::msgbox(const char* msg)
{
	Gtk::MessageDialog dlg(*_window.operator->(), msg);
	dlg.run();
}

inline void Chess::refresh()
{
	_drawarea->get_window()->invalidate(false);
	_drawarea->get_window()->process_all_updates();
}

inline void Chess::playsound(const char* filename)
{
#ifdef __linux__
	if (_voice)
	{
		char path_buf[PATH_MAX] = {0};
		//strcpy(path_buf, _exe_path.c_str());
		strcpy(path_buf, "/usr/share/cnchess/sound/");
		strcat(path_buf, filename);
		__pid_t id = fork();
		if (id == 0)
		{
			execlp("play", "play", "-q", path_buf, (char*)0);
			exit(0);
		}
	}
#else
#error "Not implement ..."
#endif
}

bool Chess::on_drawarea_button_press_event(GdkEventButton *event)
{
	if (_engine->_board._player != _player || _game_over)
		return true;

	// 首先要计算选中的交叉点
	if ((int)event->x % _cell_size > _cell_size * 0.4 &&
			(int)event->x % _cell_size < _cell_size * 0.6)
		return false;
	if ((int)event->y % _cell_size > _cell_size * 0.4 &&
			(int)event->y % _cell_size < _cell_size * 0.6)
		return false;

	int x = (int)event->x / _cell_size +
			((int)event->x % _cell_size <= _cell_size * 0.4 ? 0 : 1);
	int y = (int)event->y / _cell_size +
			((int)event->y % _cell_size <= _cell_size * 0.4 ? 0 : 1);
	if (x < 1 || x > 9)
		return true;
	if (y < 1 || y > 10)
		return true;
	x += 2;
	y += 2;
	uint32 sel;
	if (_player == 1)
		sel = coord_idx(flip_column(x), flip_row(y));
	else
		sel = coord_idx(x, y);

	if (_board_clone[sel] & base(_player))
	{
		_select = sel;
		refresh();
	}
	else if (_select != 0) // 已经选择了一个棋子了
	{
		// 表示走棋
		if (_engine->_board.legal_move(move(_select, sel)))
		{
			uint32 mv = move(_select, sel);
			if (_engine->_board.do_move(mv))
			{
				_select = 0;
				sync();
				refresh();
				_last_move = mv;
				_select = 0;
				// 检查重复局面
				int rep = _engine->_board.rep_status(3);
				if (_engine->_board.is_mate())
				{
					// 如果分出胜负，那么播放胜负的声音，并且弹出不带声音的提示框
					playsound("win.wav");
					msgbox(_("Congratulations! You win!"));
					_game_over = true;
				}
				else if (rep > 0)
				{
					rep = _engine->_board.rep_value(rep);
					// 注意："rep"是对电脑来说的分值
					if (rep > WIN_VALUE)
					{
						playsound("loss.wav");
						msgbox(_("Sorry, you can't perpetual catch or perpetual check, you lost!"));
					}
					else if (rep < -WIN_VALUE)
					{
						playsound("win.wav");
						msgbox(_("Congratulations! Computer perpetual catch or perpetual check, You win!"));
					}
					else
					{
						playsound("draw.wav");
						msgbox(_("The game ended in draw."));
					}
					_game_over = true;
				}
				else if (_engine->_board._move_count > 100)
				{
					playsound("draw.wav");
					msgbox(_("Reached the limit steps, the game ended in draw."));
					_game_over = true;
				}
				else
				{
					// 如果没有分出胜负，那么播放将军、吃子或一般走子的声音
					if (_engine->_board.in_check())
						playsound("check.wav");
					else if (_engine->_board.captured())
						playsound("captured.wav");
					else
						playsound("move.wav");
					if (_engine->_board.captured())
					{
						_engine->_board.clear_moves();
					}
					// 轮到电脑走棋
					response();
				}
			}
			else
			{
				// 播放错误的声音
				playsound("wrong.wav");
			}
		}
	}

	return true;
}

inline void Chess::response()
{
	_statusbar->push(_("Computer is thinking..."));
	_computing = true;
	Glib::Thread::create(
			sigc::mem_fun(*this, &Chess::response_move),
			0, false, true, Glib::THREAD_PRIORITY_LOW);
}

void Chess::response_move(void)
{
	int rep;
	// 电脑走一步棋
	gdk_threads_enter();
	Gdk::Cursor cur(Gdk::WATCH);
	_window->get_window()->set_cursor(cur);
	_window->get_window()->process_all_updates();
	gdk_threads_leave();

	switch (_ai_level)
	{
	case 1:
		_engine->search_main(0.1);
		break;
	case 2:
		_engine->search_main(1);
		break;
	case 3:
		_engine->search_main(3);
	}

	if (_ai_level == 1)
		usleep(800000);

	gdk_threads_enter();
	_statusbar->pop();
	_window->get_window()->set_cursor();
	_window->get_window()->process_all_updates();
	gdk_threads_leave();

	_engine->_board.do_move(_engine->_mv_result);
	sync();

	gdk_threads_enter();
	refresh();
	gdk_threads_leave();

	// 把电脑走的棋标记出来
	_last_move = _engine->_mv_result;

	// 检查重复局面
	rep = _engine->_board.rep_status(3);
	if (_engine->_board.is_mate())
	{
		// 如果分出胜负，那么播放胜负的声音，并且弹出不带声音的提示框
		playsound("loss.wav");
		gdk_threads_enter();
		msgbox(_("Sorry, you lost!"));
		gdk_threads_leave();
		_game_over = true;
	}
	else if (rep > 0)
	{
		rep = _engine->_board.rep_value(rep);
		// 注意："rep"是对玩家来说的分值
		if (rep < -WIN_VALUE)
		{
			playsound("loss.wav");
			gdk_threads_enter();
			msgbox(_("Sorry, you can't perpetual catch or perpetual check, you lost!"));
			gdk_threads_leave();
		}
		else if (rep > WIN_VALUE)
		{
			playsound("win.wav");
			gdk_threads_enter();
			msgbox(_("Congratulations! Computer perpetual catch or perpetual check, You win!"));
			gdk_threads_leave();
		}
		else
		{
			playsound("draw.wav");
			gdk_threads_enter();
			msgbox(_("The game ended in draw."));
			gdk_threads_leave();
		}
		_game_over = true;
	}
	else if (_engine->_board._move_count > 100)
	{
		playsound("draw.wav");
		gdk_threads_enter();
		msgbox(_("Reached the limit steps, the game ended in draw."));
		gdk_threads_leave();
		_game_over = true;
	}
	else
	{
		// 如果没有分出胜负，那么播放将军、吃子或一般走子的声音
		if (_engine->_board.in_check())
			playsound("check.wav");
		else if (_engine->_board.captured())
			playsound("captured.wav");
		else
			playsound("move.wav");
		if (_engine->_board.captured())
		{
			_engine->_board.clear_moves();
		}
	}
	_computing = false;
}


int main(int argc, char **argv)
{
	setlocale (LC_MESSAGES, "");
	bindtextdomain (PACKAGE, LOCALEDIR);
	bind_textdomain_codeset(PACKAGE, "UTF-8");
	textdomain (PACKAGE);

	Gtk::Main kit(argc, argv);
	Glib::thread_init(NULL);
	gdk_threads_init();

	Chess chess(argc, argv);
	chess.run();

	return 0;
}
