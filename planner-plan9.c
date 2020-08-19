#include <u.h>
#include <libc.h>
#include <draw.h>
#include <thread.h>
#include <mouse.h>
#include <frame.h>
#include <keyboard.h>
#define BUFLEN 1024

Image *back;
Rectangle **daybuttons;
int **mdays;
int *monthlens;
long t;
char monthday = 0;
Rectangle *buttons;
char isleap = 0;
Frame *text;
Image *cols[NCOL];
Mousectl *mc;
Keyboardctl *kc;
Rune *contents = nil;
int contentslen = 0;
int before = 0;
int after = 0;
int half;
int height;
int charwidth;
int charheight;

void
drawtext(void)
{
	Rectangle textr = Rpt(addpt(screen->r.min, Pt(half, 0)), screen->r.max);

	draw(screen, textr, display->white, nil, ZP);
	frclear(text, 0);
	frinit(text, textr, display->defaultfont, screen, cols);
	frinsert(text, &contents[before], &contents[contentslen], text->p1);

	after = contentslen - before - text->nchars;
}

void
scrollup(int lines)
{
	int l = lines;

	while(l > 0 && before > 0) {
		before--;
		if (contents[before] == L'\n')
			l--;
	}
	while (before > 0 && contents[before-1] != L'\n')
		before--;

	frtick(text, frptofchar(text, text->p1), 0);
	drawtext();
	frtick(text, frptofchar(text, text->p1), 0);
	text->p0 = text->p1 = 0;
	frtick(text, frptofchar(text, text->p1), 1);
	flushimage(display, 1);
}

void
scrolldown(int lines)
{
	int l = lines;

	while(l > 0 && before < contentslen) {
		if (contents[before++] == L'\n')
			l--;
	}

	frtick(text, frptofchar(text, text->p1), 0);
	drawtext();
	frtick(text, frptofchar(text, text->p1), 1);
	flushimage(display, 1);
}


int
calccharwidth(void)
{
	int i;
	Fontchar *fc;
	int w = 0;

	for (i = 0; i <= display->defaultsubfont->n; i++) {
		fc = &display->defaultsubfont->info[i];
		if (fc->width > w)
			w = fc->width;
	}

	return w;
}

void
exitall(char *arg)
{
	closekeyboard(kc);
	closemouse(mc);
	threadexitsall(arg);
}

void*
emalloc(unsigned int n)
{
	void *p;

	p = malloc(n);
	if (p == nil) {
		fprint(2, "planner: malloc failed: %r\n");
		exitall("malloc");
	}

	return p;
}

void
drawday(Image *screen, int x, int y, int mday, Image *bg, Image *fg)
{
	char *buf = emalloc(BUFLEN);
	int fd;
	Tm *tm = localtime(t);
	int year = tm->year + 1900;
	int r, i;
	Rune rune;

	draw(screen, daybuttons[x][y], bg, nil, ZP);
	snprint(buf, BUFLEN-1, "%d", mday);
	string(screen, daybuttons[x][y].min, fg, ZP, display->defaultfont, buf);
	mdays[x][y] = mday;
	snprint(buf, BUFLEN-1, "%s/lib/plans/%d/%02d/%02d/index.md", getenv("home"), year, tm->mon+1, mday);
	fd = open(buf, OREAD);
	if (fd > 0) {
		r = read(fd, buf, 3 * UTFmax + 1);
		if (r > 0) {
			buf[r] = '\0';
			buf[strcspn(buf, "\r\n")] = '\0';
			for(r = 0, i = 0, rune = L'\0'; rune != Runeerror && i < 3 && buf[r] != 0; r += chartorune(&rune, buf + r), i++);
			if (rune == Runeerror)
				r--;
			buf[r] = '\0';
			_string(screen, addpt(daybuttons[x][y].min, Pt(0, charheight)), fg, ZP, display->defaultfont, buf, nil, r, daybuttons[x][y], bg, ZP, SoverD);
		}
		close(fd);
	}

	free(buf);
}

void
updateday(Image *screen)
{
	int x, y;
	int mday;

	for (y = 0; y < 6; y++) {
		for (x = 0; x < 7; x++) {
			mday = mdays[x][y];
			if (mday == monthday) {
				drawday(screen, x, y, mday, display->black, display->white);
			}
		}
	}

	flushimage(display, 1);
}

void
resize(int w, int h)
{
	int fd;

	fd = open("/dev/wctl", OWRITE);
	if(fd >= 0){
		fprint(fd, "resize -dx %d -dy %d", w, h);
		close(fd);
	}

}

void
updateall(Image *screen)
{
	int x, y;
	char *buf;
	int year;
	int wdaystart;
	int mday;
	Tm *tm = localtime(t);
	monthday = tm->mday;
	Dir *dir;
	int fd;
	int r, i;
	char *weekdays[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
	char *months[] = { "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December" };
	Rectangle textr = Rpt(addpt(screen->r.min, Pt(half, 0)), screen->r.max);
	Rune *runes;
	Point todaysize = Pt(5 * charwidth, charheight);
	Point movesize = Pt(2 * charwidth, charheight);
	Point monthsize = Pt(9 * charwidth, charheight);
	Point yearsize = Pt(4 * charwidth, charheight);
	Point tmppt;

	x = Dx(screen->r);
	y = Dy(screen->r);
	if (x < half * 2 || y < height) {
		if (x < half * 2)
			x = half * 2 + 40;
		if (y < height)
			y = height + 40;

		resize(x, y);
		return;
	}

	buf = emalloc(BUFLEN);
	runes = emalloc(BUFLEN * sizeof(Rune));

	snprint(buf, BUFLEN-1, "%s/lib/plans", getenv("home"));
	dir = dirstat(buf);
	if (dir == nil) {
		fd = create(buf, OREAD, 0775L | DMDIR);
	} else if(!(dir->mode & DMDIR)) {
		fd = -1;
	} else {
		fd = open(buf, OREAD);
	}
	if (fd < 0) {
		fprint(2, "error opening plans directory: %r\n");
		exitall("open");
	}
	close(fd);
	
	year = tm->year + 1900;
	monthlens[1] = 28;
	isleap = (year % 4 == 0) && ((year % 100 != 0) || (year % 400 == 0));
	if (isleap)
		monthlens[1] = 29;

	tmppt = addpt(screen->r.min, Pt(5, 5));
	buttons[0] = Rpt(tmppt, addpt(tmppt, todaysize));
	tmppt.x += todaysize.x + 5;
	buttons[1] = Rpt(tmppt, addpt(tmppt, movesize));
	tmppt.x += movesize.x + monthsize.x;
	buttons[2] = Rpt(tmppt, addpt(tmppt, movesize));
	tmppt.x += movesize.x + 5;
	buttons[3] = Rpt(tmppt, addpt(tmppt, movesize));
	tmppt.x += movesize.x + yearsize.x;
	buttons[4] = Rpt(tmppt, addpt(tmppt, movesize));

	for (y = 0; y < 6; y++) {
		for (x = 0; x < 7; x++) {
			daybuttons[x][y] = rectaddpt(insetrect(Rect(x * (half/7), y * ((height - charheight * 2 - 10)/6), (x+1) * (half/7), (y+1) * ((height - charheight * 2 - 10)/6)), 5), Pt(screen->r.min.x, screen->r.min.y + charheight * 2 + 10));
		}
	}

	draw(screen, screen->r, back, nil, ZP);
	draw(screen, textr, display->white, nil, ZP);
	flushimage(display, 1);
	contentslen = 0;

	snprint(buf, BUFLEN-1, "%s/lib/plans/%d/%02d/%02d/index.md", getenv("home"), year, tm->mon+1, tm->mday);
	fd = open(buf, OREAD);
	if (fd > 0) {
		while((r = read(fd, buf, BUFLEN-1)) > 0) {
			for(x = 0, y = 0; x < r;) {
				x += chartorune(&runes[y], &buf[x]);
				if (runes[y] != Runeerror)
					y++;
			}
			contents = realloc(contents, (contentslen + y + 1) * sizeof(Rune));
			memcpy(&contents[contentslen], runes, y * sizeof(Rune));
			contentslen += y;
		}
		close(fd);
	}

	before = 0;
	drawtext();

	draw(screen, buttons[0], display->white, nil, ZP);
	string(screen, buttons[0].min, display->black, ZP, display->defaultfont, "Today");
	draw(screen, buttons[1], display->white, nil, ZP);
	string(screen, buttons[1].min, display->black, ZP, display->defaultfont, "<<");
	draw(screen, buttons[2], display->white, nil, ZP);
	string(screen, buttons[2].min, display->black, ZP, display->defaultfont, ">>");
	draw(screen, buttons[3], display->white, nil, ZP);
	string(screen, buttons[3].min, display->black, ZP, display->defaultfont, "<<");
	draw(screen, buttons[4], display->white, nil, ZP);
	string(screen, buttons[4].min, display->black, ZP, display->defaultfont, ">>");

	string(screen, addpt(buttons[1].min, Pt(2 * charwidth, 0)), display->black, ZP, display->defaultfont, months[tm->mon]);
	snprint(buf, BUFLEN-1, "%d", year);
	string(screen, addpt(buttons[3].min, Pt(2 * charwidth, 0)), display->black, ZP, display->defaultfont, buf);
	wdaystart = (8 + tm->wday - (tm->mday % 7)) % 7;

	for(x = 0; x < 7; x++) {
		string(screen, addpt(daybuttons[x][0].min, Pt(0, -charheight)), display->black, ZP, display->defaultfont, weekdays[x]);
	}
	flushimage(display, 1);

	mday = 0;
	for (y = 0; y < 6; y++) {
		for (x = 0; x < 7; x++) {
			mdays[x][y] = 0;
			if (y == 0 && wdaystart == x)
				mday = 1;
			if (mday > monthlens[tm->mon])
				mday = 0;
			if (mday != 0) {
				mdays[x][y] = mday;
				drawday(screen, x, y, mday, mday == monthday? display->black: display->white, mday == monthday? display->white: display->black);
				mday++;
			}
		}
	}

	flushimage(display, 1);
	free(buf);
	free(runes);
}

void
resizethread(void *)
{
	ulong dummy;

	while(recv(mc->resizec, &dummy) > 0){
		if(getwindow(display, Refnone) < 0)
			sysfatal("resize failed: %r");
		updateall(screen);
	}
}

void save(void);

void
keyboardthread(void *)
{
	Rune r[2];
	Rectangle textr;
	int i, p, l, w;
	ulong p1;

	while(recv(kc->c, r) > 0){
		textr = Rpt(addpt(screen->r.min, Pt(half, 0)), screen->r.max);

		if (r[0] == Kdel){
			exitall(nil);
		} else if (r[0] == 0) {
			continue;
		} else if (r[0] == Ksoh || r[0] == Khome) {
			if (text->p0 != text->p1)
				continue;

			for (p = text->p0; p > 0 && contents[before + p-1] != L'\n'; p--);
			frtick(text, frptofchar(text, text->p0), 0);
			text->p0 = text->p1 = p;
			frtick(text, frptofchar(text, text->p0), 1);
			flushimage(display, 1);
			continue;
		} else if (r[0] == Kenq || r[0] == Kend) {
			if (text->p0 != text->p1)
				continue;

			for (p = text->p1; p < text->nchars && contents[before + p] != L'\n'; p++);
			frtick(text, frptofchar(text, text->p1), 0);
			text->p0 = text->p1 = p;
			frtick(text, frptofchar(text, text->p1), 1);
			flushimage(display, 1);
			continue;
		} else if (r[0] == Kpgup) {
			if (text->p0 != text->p1)
				continue;

			if (before > 0)
				scrollup(text->maxlines / 2);

			frtick(text, frptofchar(text, text->p0), 0);
			text->p0 = text->p1 = 0;
			frtick(text, frptofchar(text, text->p0), 1);
			flushimage(display, 1);
			continue;
		} else if (r[0] == Kpgdown) {
			if (text->p0 != text->p1)
				continue;

			if (after > 0)
				scrolldown(text->maxlines / 2);

			frtick(text, frptofchar(text, text->p0), 0);
			text->p0 = text->p1 = text->nchars;
			frtick(text, frptofchar(text, text->p0), 1);
			flushimage(display, 1);
			continue;
		} else if (r[0] == Kbs) {
			if (text->p1 == 0)
				continue;
			if (text->p0 == text->p1)
				text->p0--;

			memmove(&contents[before + text->p0], &contents[before + text->p1], (contentslen - before - text->p1) * sizeof(Rune));
			contentslen -= text->p1 - text->p0;
			frtick(text, frptofchar(text, text->p1), 0);
			frdelete(text, text->p0, text->p1);
			p1 = text->p1;
			drawtext();
			frtick(text, frptofchar(text, text->p1), 0);
			text->p0 = text->p1 = p1;
			frtick(text, frptofchar(text, text->p1), 1);
		} else if (r[0] == Kleft) {
			if (text->p0 != text->p1)
				continue;

			if (text->p1 == 0) {
				if (before > 0)
					scrollup(1);
				continue;
			}

			frtick(text, frptofchar(text, text->p1), 0);
			text->p1--;
			text->p0--;
			frtick(text, frptofchar(text, text->p1), 1);
			flushimage(display, 1);
			continue;
		} else if (r[0] == Kright) {
			if (text->p0 != text->p1)
				continue;

			if (text->p1 == text->nchars) {
				if (after > 0)
					scrolldown(1);
				continue;
			}

			frtick(text, frptofchar(text, text->p1), 0);
			text->p1++;
			text->p0++;
			frtick(text, frptofchar(text, text->p1), 1);
			flushimage(display, 1);
			continue;
		} else if (r[0] == Kup) {
			if (text->p0 != text->p1)
				continue;

			for (p = text->p0, l = 0; p > 0 && contents[before + p-1] != L'\n'; p--, l++);
			if (p == 0) {
				if (before > 0)
					scrollup(1);
				continue;
			}

			for (i = p - 1, w = 0; i > 0 && contents[before + i-1] != L'\n'; i--, w++);
			frtick(text, frptofchar(text, text->p0), 0);
			text->p0 = text->p1 = l > w? i + w: i + l;
			frtick(text, frptofchar(text, text->p0), 1);
			flushimage(display, 1);
			continue;
		} else if (r[0] == Kdown) {
			if (text->p1 != text->p0)
				continue;

			for (p = text->p0, l = 0; p > 0 && contents[before + p-1] != L'\n'; p--, l++);
			for (p = text->p0; p < text->nchars && contents[before + p] != L'\n'; p++);
			if (p == text->nchars) {
				if (after > 0)
					scrolldown(1);
				continue;
			}

			for (i = p + 1, w = 0; i < text->nchars && contents[before + i] != L'\n'; i++, w++);
			frtick(text, frptofchar(text, text->p0), 0);
			text->p0 = text->p1 = l > w? p + w + 1: p + l + 1;
			frtick(text, frptofchar(text, text->p0), 1);
			flushimage(display, 1);
			continue;
		} else {
			if (text->p0 != text->p1) {
				memmove(&contents[before + text->p0], &contents[before + text->p1], (contentslen - before - text->p1) * sizeof(Rune));
				contentslen -= text->p1 - text->p0;
				frdelete(text, text->p0, text->p1);
			}

			contentslen++;
			contents = realloc(contents, (contentslen+1) * sizeof(Rune));
			memmove(&contents[before + text->p1 + 1], &contents[before + text->p1], (contentslen - before - text->p1 - 1) * sizeof(Rune));
			contents[before + text->p1] = r[0];
			frinsert(text, &r[0], &r[1], text->p1);
		}

		flushimage(display, 1);
		save();
	}
}

void
chkdir(char *fname) {
	Dir *dir;
	int fd;

	dir = dirstat(fname);
	if (dir == nil) {
		fd = create(fname, OREAD, 0775L | DMDIR);
		if (fd > 0) {
			close(fd);
		} else {
			fprint(2, "unable to create %s: %r\n", fname);
			exitall("create");
		}
	}
}

void
save(void)
{
	char *buf;
	int i, p, l, w;
	int fd;
	Tm *tm;
	char *fname = emalloc(BUFLEN);
	Rune r[2];

	buf = emalloc(contentslen * UTFmax);
	p = 0;
	fd = 1;
	for (i = 0; i < contentslen; i++) {
		w = runetochar(&buf[p], &contents[i]);
		p += w;
	}

	tm = localtime(t);

	snprint(fname, BUFLEN-1, "%s/lib/plans/%d", getenv("home"), tm->year + 1900);
	chkdir(fname);

	snprint(fname, BUFLEN-1, "%s/lib/plans/%d/%02d", getenv("home"), tm->year + 1900, tm->mon + 1);
	chkdir(fname);

	snprint(fname, BUFLEN-1, "%s/lib/plans/%d/%02d/%02d", getenv("home"), tm->year + 1900, tm->mon + 1, tm->mday);
	chkdir(fname);

	snprint(fname, BUFLEN-1, "%s/lib/plans/%d/%02d/%02d/index.md", getenv("home"), tm->year + 1900, tm->mon + 1, tm->mday);
	fd = create(fname, OWRITE, 0664L);
	if (fd > 0) {
		i = 0;
		while (i < p) {
			w = write(fd, buf + i, p - i);
			if (w < 0) {
				fprint(2, "write error to %s: %r\n", fname);
				exitall("write");
			}
			i += w;
		}
		close(fd);
	} else {
		fprint(2, "unable to create %s: %r\n", fname);
		exitall("create");
	}

	free(buf);

	if (fd < 0) {
		fprint(2, "error saving file: %s\n", fname);
		exitall("write");
	}

	updateday(screen);
	free(fname);
}

void
usage(void)
{
	fprint(2, "usage: planner [-c]\n");
	threadexitsall("usage");
}

void
tosnarf(void)
{
	int fd = open("/dev/snarf", OWRITE);
	char *buf;
	int l;
	int i, r, a;

	if (fd < 0) {
		fprint(2, "open /dev/snarf: %r\n");
		return;
	}

	buf = emalloc(BUFLEN);

	for (i = text->p0; i < text->p1;) {
		a = 0;
		while(a < (BUFLEN-UTFmax) && i < text->p1) {
			l = runetochar(buf + a, &contents[before + i++]);
			a += l;
		}
		l = 0;
		while(a > 0) {
			r = write(fd, buf + l, a);
			if (r > 0) {
				a -= r;
				l += r;
			} else {
				fprint(2, "write: /dev/snarf: %r\n");
				break;
			}
		}
	}

	close(fd);
	free(buf);
}

void
threadmain(int argc, char **argv)
{
	Mouse m;
	int x, y;
	Tm *tm;
	Rectangle textr;
	Menu menu;
	char *mstr[] = {"cut", "snarf", "paste", "exit", 0};
	int fd;
	char *buf;
	int r, l, i, s, start;
	long click = 0;
	long clickcount = 0;
	int dt;
	ulong p;
	Point caldims;

	if(initdraw(0,0,"planner") < 0)
		sysfatal("initdraw: %r");
	back = allocimagemix(display, DGreen, DWhite);
	mc = initmouse(nil, screen);
	if (mc == nil)
		sysfatal("initmouse: %r");
	kc = initkeyboard(nil);
	if (kc == nil)
		sysfatal("initkeyboard: %r");

	buttons = emalloc(5 * sizeof(Rectangle));
	mdays = emalloc(7 * sizeof(int*));
	daybuttons = emalloc(7 * sizeof(Rectangle*));
	for (x = 0; x < 7; x++) {
		mdays[x] = emalloc(6 * sizeof(int));
		daybuttons[x] = emalloc(6 * sizeof(Rectangle));
	}
	monthlens = emalloc(12 * sizeof(int));
	monthlens[8] = monthlens[3] = monthlens[5] = monthlens[10] = 30;
	monthlens[0] = monthlens[2] = monthlens[4] = monthlens[6] = monthlens[7] = monthlens[9] = monthlens[11] = 31;
	monthlens[1] = 28;

	charwidth = calccharwidth();
	charheight = display->defaultsubfont->height;
	half = charwidth * 21 + 5 * 14;
	height = charheight * 14 + 5 * 13;
	t = time(nil);

	buf = emalloc(BUFLEN);
	contents = emalloc(1 * sizeof(Rune));
	contents[0] = L'\0';

	cols[TEXT] = display->black;
	cols[BACK] = display->white;
	cols[HIGH] = display->black;
	cols[HTEXT] = display->white;
	cols[BORD] = display->black;
	text = calloc(1, sizeof(Frame));
	textr = Rpt(addpt(screen->r.min, Pt(half, 0)), screen->r.max);
	frinit(text, textr, display->defaultfont, screen, cols);

	threadcreate(resizethread, nil, mainstacksize);
	threadcreate(keyboardthread, nil, mainstacksize);
	updateall(screen);

	menu.item = mstr;
	menu.lasthit = 0;
	while(recv(mc->c, &m) >= 0) {
		if (m.buttons & 4) {
			x = menuhit(3, mc, &menu, nil);
			switch(x) {
			case 0:
				if (text->p0 == text->p1)
					continue;
				tosnarf();
				memmove(&contents[before + text->p0], &contents[before + text->p1], (contentslen - before - text->p1) * sizeof(Rune));
				contentslen -= text->p1 - text->p0;
				frdelete(text, text->p0, text->p1);
				p = text->p1;
				frtick(text, frptofchar(text, text->p1), 0);
				drawtext();
				frtick(text, frptofchar(text, text->p1), 0);
				text->p0 = text->p1 = p;
				frtick(text, frptofchar(text, text->p1), 1);
				flushimage(display, 1);
				save();
				break;
			case 1:
				if (text->p0 == text->p1)
					continue;
				tosnarf();
				break;
			case 2:
				fd = open("/dev/snarf", OREAD);
				if (fd < 0) {
					fprint(2, "open /dev/snarf: %r\n");
					continue;
				}

				if (text->p0 != text->p1) {
					memmove(&contents[before + text->p0], &contents[before + text->p1], (contentslen - before - text->p1) * sizeof(Rune));
					contentslen -= text->p1 - text->p0;
					frdelete(text, text->p0, text->p1);
				}
				frtick(text, frptofchar(text, text->p1), 0);

				y = 0;
				start = s = text->p1;
				while ((r = read(fd, buf + y, BUFLEN - y)) > 0) {
					l = utfnlen(buf, r);
					y = r;
					contentslen += l;
					contents = realloc(contents, (contentslen+1) * sizeof(Rune));
					memmove(&contents[before + s + l], &contents[before + s], (contentslen - before - (s + l)) * sizeof(Rune));
					i = 0;
					x = 0;
					while(x < l) {
						r = chartorune(&contents[before + s + x], buf + i);
						x++;
						i += r;
						y -= r;
					}
					memmove(buf, buf + i, y);
					s += l;
				}
				if (r < 0)
					fprint(2, "read: /dev/snarf: %r\n");
				else
					frinsert(text, &contents[before + start], &contents[before + s], start);
				close(fd);
				p = text->p1;
				frtick(text, frptofchar(text, text->p1), 0);
				drawtext();
				frtick(text, frptofchar(text, text->p1), 0);
				text->p0 = text->p1 = p;
				frtick(text, frptofchar(text, text->p1), 1);
				flushimage(display, 1);
				save();
				break;
			case 3:
				exitall(nil);
			default:
				break;	
			}
			continue;
		}

		textr = Rpt(Pt(screen->r.min.x + half, screen->r.min.y), screen->r.max);

		if (m.buttons & 8 && ptinrect(m.xy, textr)) {
			s = m.xy.y - textr.min.y;
			s /= charheight;
			scrollup(s);
		}

		if (m.buttons & 16 && ptinrect(m.xy, textr)) {
			s = m.xy.y - textr.min.y;
			s /= charheight;
			scrolldown(s);
		}

		if (!(m.buttons & 1))
			continue;

		if (ptinrect(m.xy, buttons[0])) {
			t = time(nil);
			updateall(screen);
			continue;
		}

		if (ptinrect(m.xy, buttons[1])) {
			t -= monthday * 24 * 3600;
			tm = localtime(t);
			if (monthday > monthlens[tm->mon])
				monthday = monthlens[tm->mon];
			t -= (monthlens[tm->mon] - monthday) * 24 * 3600;
			updateall(screen);
			continue;
		}

		if (ptinrect(m.xy, buttons[2])) {
			tm = localtime(t);
			t += (monthlens[tm->mon] - monthday + 1) * 24 * 3600;
			tm = localtime(t);
			if (monthday > monthlens[tm->mon])
				monthday = monthlens[tm->mon];
			t += (monthday - 1) * 24 * 3600;
			updateall(screen);
			continue;
		}

		if (ptinrect(m.xy, buttons[3])) {
			t -= (isleap? 366: 365) * 24 * 3600;
			updateall(screen);
			continue;
		}

		if (ptinrect(m.xy, buttons[4])) {
			t += (isleap? 366: 365) * 24 * 3600;
			updateall(screen);
			continue;
		}

		for (y = 0; y < 6; y++) {
			for (x = 0; x < 7; x++) {
				if (mdays[x][y] == 0)
					continue;

				if (ptinrect(m.xy, daybuttons[x][y])) {
					t += (mdays[x][y] - monthday) * 24 * 3600;
					updateall(screen);
				}
			}
		}

		if (ptinrect(m.xy, textr)) {
			dt = m.msec - click;
			click = m.msec;
			if (dt < 500)
				clickcount++;
			else {
				clickcount = 0;
				frselect(text, mc);
			}
			if (clickcount > 1) {
				while(text->p0 > 0 && contents[before + text->p0 - 1] != L'\n') text->p0--;
				while(text->p1 < text->nchars && contents[before + text->p1-1] != L'\n') text->p1++;
				frdrawsel(text, frptofchar(text, text->p0), text->p0, text->p1, 1);
				flushimage(display, 1);
			} else if (clickcount > 0) {
				frtick(text, frptofchar(text, text->p1), 0);
				while(text->p0 > 0 && !isspacerune(contents[before + text->p0 - 1])) text->p0--;
				while(text->p1 < text->nchars && !isspacerune(contents[before + text->p1])) text->p1++;
				frdrawsel(text, frptofchar(text, text->p0), text->p0, text->p1, 1);
				flushimage(display, 1);
			}
		}
	}
}
