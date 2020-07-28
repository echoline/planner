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
Channel *keypressed;

void*
emalloc(unsigned int n)
{
	void *p;

	p = malloc(n);
	if (p == nil) {
		fprint(2, "planner: malloc failed: %r\n");
		closekeyboard(kc);
		closemouse(mc);
		threadexitsall("malloc");
	}

	return p;
}

void
updateday(Image *screen)
{
	int x, y;
	int mday;
	char *buf = emalloc(BUFLEN);
	int fd;
	Tm *tm = localtime(t);
	int year = tm->year + 1900;
	int r;

	for (y = 0; y < 6; y++) {
		for (x = 0; x < 7; x++) {
			mday = mdays[x][y];
			if (mday == monthday) {
				draw(screen, daybuttons[x][y], display->black, nil, ZP);
				snprint(buf, BUFLEN-1, "%d", mday);
				string(screen, daybuttons[x][y].min, display->white, ZP, display->defaultfont, buf);
				mdays[x][y] = mday;
				snprint(buf, BUFLEN-1, "%s/lib/plans/%d/%02d/%02d/index.txt", getenv("home"), year, tm->mon+1, mday);
				fd = open(buf, OREAD);
				if (fd > 0) {
					r = read(fd, buf, 3);
					if (r > 0) {
						buf[r] = '\0';
						buf[strcspn(buf, "\r\n")] = '\0';
						string(screen, addpt(daybuttons[x][y].min, Pt(0, Dy(buttons[0]))), display->white, ZP, display->defaultfont, buf);
					}
					close(fd);
				}
				mday++;
			}
		}
	}

	free(buf);
	flushimage(display, 1);
}

void
updateall(Image *screen)
{
	int height = Dy(screen->r) > 512? 512: Dy(screen->r);
	int half = Dx(screen->r)/2;
	int x, y;
	char *buf = emalloc(BUFLEN);
	int year;
	int wdaystart;
	int mday;
	Tm *tm = localtime(t);
	monthday = tm->mday;
	Point tmppt;
	Dir *dir;
	int fd;
	int r, i;
	char *weekdays[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
	char *months[] = { "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December" };
	Point todaysize;
	Point movesize;
	Point monthsize;
	Point yearsize;
	Rectangle textr = Rpt(addpt(screen->r.min, Pt(half, 0)), screen->r.max);
	Rune *runes = emalloc(BUFLEN * sizeof(Rune));

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
		fprint(2, "error opening plans directory\n");
		closekeyboard(kc);
		closemouse(mc);
		threadexitsall("error opening plans directory");
	}
	close(fd);

	todaysize = stringsize(display->defaultfont, "Today");
	movesize = stringsize(display->defaultfont, "<<");
	monthsize = stringsize(display->defaultfont, "September"); // longest month name in any font hopefully
	year = tm->year + 1900;
	monthlens[1] = 28;
	isleap = (year % 4 == 0) && ((year % 100 != 0) || (year % 400 == 0));
	if (isleap)
		monthlens[1] = 29;
	snprint(buf, BUFLEN-1, "%d", year);
	yearsize = stringsize(display->defaultfont, buf);

	tmppt = addpt(screen->r.min, Pt(10, 10));
	buttons[0] = Rpt(tmppt, addpt(tmppt, todaysize));

	tmppt.x += todaysize.x + 20;
	buttons[1] = Rpt(tmppt, addpt(tmppt, movesize));

	tmppt.x += movesize.x + monthsize.x;
	buttons[2] = Rpt(tmppt, addpt(tmppt, movesize));

	tmppt.x += movesize.x + 20;
	buttons[3] = Rpt(tmppt, addpt(tmppt, movesize));

	tmppt.x += movesize.x + yearsize.x;
	buttons[4] = Rpt(tmppt, addpt(tmppt, movesize));

	for (y = 0; y < 6; y++) {
		for (x = 0; x < 7; x++) {
			daybuttons[x][y] = rectaddpt(insetrect(Rect(x * ((half-20)/7), y * ((height-70)/6), (x+1) * ((half-20)/7), (y+1) * ((height-70)/6)), 10), Pt(screen->r.min.x, screen->r.min.y + 50));
		}
	}

	draw(screen, screen->r, back, nil, ZP);

	frclear(text, 0);
	draw(screen, textr, display->white, nil, ZP);
	frinit(text, textr, display->defaultfont, screen, cols);
	contentslen = 0;

	snprint(buf, BUFLEN-1, "%s/lib/plans/%d/%02d/%02d/index.txt", getenv("home"), year, tm->mon+1, tm->mday);
	fd = open(buf, OREAD);
	if (fd > 0) {
		while((r = read(fd, buf, BUFLEN-1)) > 0) {
			for(x = 0, y = 0; x < r;) {
				x += chartorune(&runes[y], &buf[x]);
				y++;
			}
			frinsert(text, runes, &runes[y], text->p1);
			contents = realloc(contents, (contentslen + y) * sizeof(Rune));
			memcpy(&contents[contentslen], runes, y * sizeof(Rune));
			contentslen += y;
		}
		close(fd);
	}

	snprint(buf, BUFLEN-1, "%d", year);
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

	string(screen, addpt(buttons[1].min, Pt(movesize.x, 0)), display->black, ZP, display->defaultfont, months[tm->mon]);
	string(screen, addpt(buttons[3].min, Pt(movesize.x, 0)), display->black, ZP, display->defaultfont, buf);
	wdaystart = (8 + tm->wday - (tm->mday % 7)) % 7;
	mday = 0;

	for(x = 0; x < 7; x++) {
		string(screen, addpt(daybuttons[x][0].min, Pt(0, -todaysize.y)), display->black, ZP, display->defaultfont, weekdays[x]);
	}

	for (y = 0; y < 6; y++) {
		for (x = 0; x < 7; x++) {
			mdays[x][y] = 0;
			if (y == 0 && wdaystart == x)
				mday = 1;
			if (mday > monthlens[tm->mon])
				mday = 0;
			if (mday != 0) {
				draw(screen, daybuttons[x][y], mday == monthday? display->black: display->white, nil, ZP);
				snprint(buf, BUFLEN-1, "%d", mday);
				string(screen, daybuttons[x][y].min, mday == monthday? display->white: display->black, ZP, display->defaultfont, buf);
				mdays[x][y] = mday;

				snprint(buf, BUFLEN-1, "%s/lib/plans/%d/%02d/%02d/index.txt", getenv("home"), year, tm->mon+1, mday);
				fd = open(buf, OREAD);
				if (fd > 0) {
					r = read(fd, buf, 3);
					if (r > 0) {
						buf[r] = '\0';
						buf[strcspn(buf, "\r\n")] = '\0';
						string(screen, addpt(daybuttons[x][y].min, Pt(0, todaysize.y)), mday == monthday? display->white: display->black, ZP, display->defaultfont, buf);
					}
					close(fd);
				}

				mday++;
			}
		}
	}

	free(buf);
	free(runes);
	flushimage(display, 1);
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
savethread(void*)
{
	ulong dummy;
	ulong last = time(nil);

	while(recv(keypressed, &dummy) > 0){
		if (last < (time(nil) - 3)) {
			save();
			last = time(nil);
		}
	}
}

void
keyboardthread(void *)
{
	Rune r[2];
	int half;
	Rectangle textr;
	int i, p, l, w;
	ulong dummy = 1;

	while(recv(kc->c, r) > 0){
		half = Dx(screen->r)/2;
		textr = Rpt(addpt(screen->r.min, Pt(half, 0)), screen->r.max);

		if (r[0] == Kdel){
			closekeyboard(kc);
			closemouse(mc);
			closedisplay(display);
			threadexitsall(nil);
		} else if (r[0] == 0) {
			continue;
		} else if (r[0] == Kbs) {
			if (text->p1 == 0)
				continue;
			if (text->p0 == text->p1)
				text->p0--;

			memmove(&contents[text->p0], &contents[text->p1], (contentslen - text->p1) * sizeof(Rune));
			contentslen -= text->p1 - text->p0;
			frtick(text, frptofchar(text, text->p1), 0);
			frdelete(text, text->p0, text->p1);
			frtick(text, frptofchar(text, text->p1), 1);
		} else if (r[0] == Kleft && text->p1 == text->p0) {
			if (text->p1 == 0)
				continue;

			frtick(text, frptofchar(text, text->p1), 0);
			text->p1--;
			text->p0--;
			frtick(text, frptofchar(text, text->p1), 1);
			flushimage(display, 1);
			continue;
		} else if (r[0] == Kright && text->p1 == text->p0) {
			if (text->p1 == text->nchars)
				continue;

			frtick(text, frptofchar(text, text->p1), 0);
			text->p1++;
			text->p0++;
			frtick(text, frptofchar(text, text->p1), 1);
			flushimage(display, 1);
			continue;
		} else if (r[0] == Kup && text->p1 == text->p0) {
			for (p = text->p0, l = 0; p > 0 && contents[p-1] != L'\n'; p--, l++);
			if (p == 0)
				continue;

			for (i = p - 1, w = 0; i > 0 && contents[i-1] != L'\n'; i--, w++);
			frtick(text, frptofchar(text, text->p0), 0);
			text->p0 = text->p1 = l > w? i + w: i + l;
			frtick(text, frptofchar(text, text->p0), 1);
			flushimage(display, 1);
			continue;
		} else if (r[0] == Kdown && text->p1 == text->p0) {
			for (p = text->p0, l = 0; p > 0 && contents[p-1] != L'\n'; p--, l++);
			for (p = text->p0; p < text->nchars && contents[p] != L'\n'; p++);
			if (p == text->nchars)
				continue;

			for (i = p + 1, w = 0; i < text->nchars && contents[i] != L'\n'; i++, w++);
			frtick(text, frptofchar(text, text->p0), 0);
			text->p0 = text->p1 = l > w? p + w + 1: p + l + 1;
			frtick(text, frptofchar(text, text->p0), 1);
			flushimage(display, 1);
			continue;
		} else {
			if (text->p0 != text->p1) {
				memmove(&contents[text->p0], &contents[text->p1], (contentslen - text->p1) * sizeof(Rune));
				contentslen -= text->p1 - text->p0;
				frdelete(text, text->p0, text->p1);
			}

			contentslen++;
			contents = realloc(contents, contentslen * sizeof(Rune));
			memmove(&contents[text->p1 + 1], &contents[text->p1], (contentslen - text->p1 - 1) * sizeof(Rune));
			contents[text->p1] = r[0];
			frinsert(text, &r[0], &r[1], text->p1);
		}

		flushimage(display, 1);
		send(keypressed, &dummy);
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
	Dir *dir;
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
	dir = dirstat(fname);
	if (dir == nil) {
		fd = create(fname, OREAD, 0775L | DMDIR);
		if (fd > 0) {
			close(fd);
		}
	}
	snprint(fname, BUFLEN-1, "%s/lib/plans/%d/%02d", getenv("home"), tm->year + 1900, tm->mon + 1);
	dir = dirstat(fname);
	if (fd > 0 && dir == nil) {
		fd = create(fname, OREAD, 0775L | DMDIR);
		if (fd > 0) {
			close(fd);
		}
	}
	snprint(fname, BUFLEN-1, "%s/lib/plans/%d/%02d/%02d", getenv("home"), tm->year + 1900, tm->mon + 1, tm->mday);
	dir = dirstat(fname);
	if (fd > 0 && dir == nil) {
		fd = create(fname, OREAD, 0775L | DMDIR);
		if (fd > 0) {
			close(fd);
		}
	}

	if (dir != nil || fd > 0) {
		snprint(fname, BUFLEN-1, "%s/lib/plans/%d/%02d/%02d/index.txt", getenv("home"), tm->year + 1900, tm->mon + 1, tm->mday);
		fd = create(fname, OWRITE, 0664L);
		if (fd > 0) {
			i = 0;
			while (i < p) {
				w = write(fd, buf + i, p - i);
				if (w < 1)
					break;
				i += w;
			}
			close(fd);
		}
	}
	free(buf);
	if (fd < 0) {
		fprint(2, "error saving file: %s\n", fname);
		closekeyboard(kc);
		closemouse(mc);
		threadexitsall("write");
	}
	updateday(screen);
	flushimage(display, 1);
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
	char *buf = emalloc(BUFLEN);
	int l;
	int i, r, a;

	if (fd < 0) {
		fprint(2, "open /dev/snarf: %r\n");
		free(buf);
		return;
	}

	for (i = text->p0; i < text->p1;) {
		a = 0;
		while(a < (BUFLEN-UTFmax) && i < text->p1) {
			l = runetochar(buf + a, &contents[i++]);
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
	int half;
	Rectangle textr;
	Menu menu;
	char *mstr[] = {"cut", "snarf", "paste", "exit", 0};
	int fd;
	char *buf;
	int r, l, i;
	ulong dummy = 1;
	long click = 0;
	long clickcount = 0;
	int dt;

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
	monthlens[0] = 31;
	monthlens[1] = 28;
	monthlens[2] = 31;
	monthlens[3] = 30;
	monthlens[4] = 31;
	monthlens[5] = 30;
	monthlens[6] = 31;
	monthlens[7] = 31;
	monthlens[8] = 30;
	monthlens[9] = 31;
	monthlens[10] = 30;
	monthlens[11] = 31;

	half = Dx(screen->r)/2;
	t = time(nil);
	buf = emalloc(BUFLEN);

	cols[TEXT] = display->black;
	cols[BACK] = display->white;
	cols[HIGH] = display->black;
	cols[HTEXT] = display->white;
	cols[BORD] = display->black;
	text = calloc(1, sizeof(Frame));
	textr = Rpt(addpt(screen->r.min, Pt(half, 0)), screen->r.max);
	frinit(text, textr, display->defaultfont, screen, cols);

	keypressed = chancreate(sizeof(ulong), 1);

	threadcreate(resizethread, nil, mainstacksize);
	threadcreate(keyboardthread, nil, mainstacksize);
	threadcreate(savethread, nil, mainstacksize);
	updateall(screen);

	menu.item = mstr;
	menu.lasthit = 0;
	while(recv(mc->c, &m) >= 0) {
		send(keypressed, &dummy);

		if (m.buttons & 4) {
			x = menuhit(3, mc, &menu, nil);
			switch(x) {
			case 0:
				if (text->p0 == text->p1)
					continue;
				tosnarf();
				memmove(&contents[text->p0], &contents[text->p1], contentslen - text->p1);
				contentslen -= text->p1 - text->p0;
				frdelete(text, text->p0, text->p1);
				send(keypressed, &dummy);
				flushimage(display, 1);
				break;
			case 1:
				if (text->p0 == text->p1)
					continue;
				tosnarf();
				break;
			case 2:
				if (text->p0 != text->p1) {
					memmove(&contents[text->p0], &contents[text->p1], contentslen - text->p1);
					contentslen -= text->p1 - text->p0;
					frdelete(text, text->p0, text->p1);
				}
				fd = open("/dev/snarf", OREAD);
				if (fd < 0) {
					fprint(2, "open /dev/snarf: %r\n");
					continue;
				}

				y = 0;
				while ((r = read(fd, buf + y, BUFLEN - y)) > 0) {
					if (r < 0) {
						fprint(2, "read: /dev/snarf: %r\n");
					}
					l = utfnlen(buf, r);
					y = r - l;
					contentslen += l;
					contents = realloc(contents, contentslen * sizeof(Rune));
					memmove(&contents[text->p1 + l], &contents[text->p1], contentslen - (text->p1 + l));
					i = 0;
					while(l > 0) {
						r = chartorune(&contents[text->p1 + i], buf + i);
						l -= r;
						i += r;
					}
					memmove(buf, buf + i, y);
				}
				frinsert(text, &contents[text->p1], &contents[text->p1 + i], text->p1);
				close(fd);
				send(keypressed, &dummy);
				flushimage(display, 1);
				break;
			case 3:
				closekeyboard(kc);
				closemouse(mc);
				threadexitsall(nil);
			default:
				break;	
			}
			continue;
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
			t += monthday * 24 * 3600;
			if (monthday > monthlens[tm->mon])
				monthday = monthlens[tm->mon];
			t += (monthlens[tm->mon] - monthday) * 24 * 3600;
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

		half = Dx(screen->r)/2;
		textr = Rpt(Pt(screen->r.min.x + half, screen->r.min.y), screen->r.max);
		if (ptinrect(m.xy, textr)) {
			dt = m.msec - click;
			click = m.msec;
			if (dt < 500)
				clickcount++;
			else {
				clickcount = 0;
				frselect(text, mc);
			}
			if (clickcount > 2) {
				while(text->p0 > 0 && contents[text->p0 - 1] != L'\n') text->p0--;
				while(text->p1 < text->nchars && contents[text->p1-1] != L'\n') text->p1++;
				frdrawsel(text, frptofchar(text, text->p0), text->p0, text->p1, 1);
				flushimage(display, 1);
			} else if (clickcount > 1) {
				while(text->p0 > 0 && !isspacerune(contents[text->p0 - 1])) text->p0--;
				while(text->p1 < text->nchars && !isspacerune(contents[text->p1])) text->p1++;
				frdrawsel(text, frptofchar(text, text->p0), text->p0, text->p1, 1);
				flushimage(display, 1);
			}
		}
	}
}
