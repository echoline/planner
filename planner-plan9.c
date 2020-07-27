#include <u.h>
#include <libc.h>
#include <draw.h>
#include <thread.h>
#include <mouse.h>
#include <frame.h>
#include <keyboard.h>

#define BUFLEN 8192

Image *back;
Rectangle **days;
int **mdays;
int *monthlens;
long t;
int monthday = 0;
Rectangle *buttons;
int isleap = 0;
Frame *text;
Image *cols[NCOL];
Mousectl *mc;
Keyboardctl *kc;
Rune *contents = nil;
int contentslen = 0;

void
updatetoday(Image *screen)
{
	int x, y;
	int mday;
	char *buf = malloc(BUFLEN);
	int fd;
	Tm *tm = localtime(t);
	int year = tm->year + 1900;
	int r;

	for (y = 0; y < 6; y++) {
		for (x = 0; x < 7; x++) {
			mday = mdays[x][y];
			if (mday == monthday) {
				draw(screen, days[x][y], display->black, nil, ZP);
				snprint(buf, BUFLEN-1, "%d", mday);
				string(screen, days[x][y].min, display->white, ZP, display->defaultfont, buf);
				mdays[x][y] = mday;
				snprint(buf, BUFLEN-1, "%s/lib/plans/%d/%02d/%02d/index.txt", getenv("home"), year, tm->mon+1, mday);
				fd = open(buf, OREAD);
				if (fd > 0) {
					r = read(fd, buf, 3);
					if (r > 0) {
						buf[r] = '\0';
						buf[strcspn(buf, "\r\n")] = '\0';
						string(screen, addpt(days[x][y].min, Pt(0, Dy(buttons[0]))), display->white, ZP, display->defaultfont, buf);
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
	char *buf = malloc(BUFLEN);
	int year;
	int wdaystart;
	int mday;
	Tm *tm = localtime(t);
	monthday = tm->mday;
	Point tmppt;
	Dir *dir;
	int fd;
	int r;
	char *weekdays[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
	char *months[] = { "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December" };
	Point todaysize;
	Point movesize;
	Point monthsize;
	Point yearsize;
	Rectangle textr = Rpt(addpt(screen->r.min, Pt(half, 0)), screen->r.max);
	Rune *runes = malloc(BUFLEN * sizeof(Rune));

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
		lockdisplay(display);
		closedisplay(display);
		exits("error opening plans directory");
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
			days[x][y] = rectaddpt(insetrect(Rect(x * ((half-20)/7), y * ((height-70)/6), (x+1) * ((half-20)/7), (y+1) * ((height-70)/6)), 10), Pt(screen->r.min.x, screen->r.min.y + 50));
		}
	}

	draw(screen, screen->r, back, nil, ZP);

	frclear(text, 0);
	draw(screen, textr, display->white, nil, ZP);
	frinit(text, textr, display->defaultfont, screen, cols);
	snprint(buf, BUFLEN-1, "%s/lib/plans/%d/%02d/%02d/index.txt", getenv("home"), year, tm->mon+1, tm->mday);
	contentslen = 0;
	fd = open(buf, OREAD);
	if (fd > 0) {
		while((r = read(fd, buf, BUFLEN-1)) > 0) {
			for(x = 0, y = 0; x < r;) {
				x += chartorune(&runes[y], &buf[x]);
				y++;
			}
			frinsert(text, runes, &runes[y], text->p1);
			contents = realloc(contents, (contentslen + y) * sizeof(Rune));
			memcpy(contents, runes, y * sizeof(Rune));
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
		string(screen, addpt(days[x][0].min, Pt(0, -todaysize.y)), display->black, ZP, display->defaultfont, weekdays[x]);
	}

	for (y = 0; y < 6; y++) {
		for (x = 0; x < 7; x++) {
			mdays[x][y] = 0;
			if (y == 0 && wdaystart == x)
				mday = 1;
			if (mday > monthlens[tm->mon])
				mday = 0;
			if (mday != 0) {
				draw(screen, days[x][y], mday == monthday? display->black: display->white, nil, ZP);
				snprint(buf, BUFLEN-1, "%d", mday);
				string(screen, days[x][y].min, mday == monthday? display->white: display->black, ZP, display->defaultfont, buf);
				mdays[x][y] = mday;
				snprint(buf, BUFLEN-1, "%s/lib/plans/%d/%02d/%02d/index.txt", getenv("home"), year, tm->mon+1, mday);
				fd = open(buf, OREAD);
				if (fd > 0) {
					r = read(fd, buf, 3);
					if (r > 0) {
						buf[r] = '\0';
						buf[strcspn(buf, "\r\n")] = '\0';
						string(screen, addpt(days[x][y].min, Pt(0, todaysize.y)), mday == monthday? display->white: display->black, ZP, display->defaultfont, buf);
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
		lockdisplay(display);
		updateall(screen);
		unlockdisplay(display);
	}
}

void
keyboardthread(void *)
{
	Rune r[2];
	char *buf;
	int i, p;
	int fd, w;
	Tm *tm = localtime(t);
	char *fname = malloc(BUFLEN);
	int half;
	Dir *dir;
	Rectangle textr;

	while(recv(kc->c, r) > 0){
		half = Dx(screen->r)/2;
		textr = Rpt(addpt(screen->r.min, Pt(half, 0)), screen->r.max);

		if (r[0] == 127){
			closekeyboard(kc);
			closemouse(mc);
			lockdisplay(display);
			closedisplay(display);
			exits(nil);
		} else if (r[0] == 0) {
			continue;
		} else if (r[0] == 8) {
			if (text->p1 == 0)
				continue;
			if (text->p0 == text->p1)
				text->p0--;

			memmove(&contents[text->p0], &contents[text->p1], (contentslen - text->p1) * sizeof(Rune));
			contentslen -= text->p1 - text->p0;
			frdelete(text, text->p0, text->p1);
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

		lockdisplay(display);
		buf = malloc(contentslen * UTFmax);
		p = 0;
		fd = 1;
		for (i = 0; i < contentslen; i++) {
			w = runetochar(&buf[p], &contents[i]);
			p += w;
		}
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
			lockdisplay(display);
			closedisplay(display);
			exits("write");
		}
		updatetoday(screen);
		flushimage(display, 1);
		unlockdisplay(display);
	}
}

void
threadmain(int argc, char **argv)
{
	Mouse m;
	int x, y;
	Tm *tm;
	int half;
	Rectangle textr;

	buttons = malloc(5 * sizeof(Rectangle));
	mdays = malloc(7 * sizeof(int*));
	days = malloc(7 * sizeof(Rectangle*));
	for (x = 0; x < 7; x++) {
		mdays[x] = malloc(6 * sizeof(int));
		days[x] = malloc(6 * sizeof(Rectangle));
	}
	monthlens = malloc(12 * sizeof(int));
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

	if(initdraw(0,0,"planner") < 0)
		sysfatal("initdraw: %r");
	back = allocimagemix(display, DGreen, DWhite);
	mc = initmouse(nil, screen);
	if (mc == nil)
		sysfatal("initmouse: %r");
	kc = initkeyboard(nil);
	if (kc == nil)
		sysfatal("initkeyboard: %r");

	half = Dx(screen->r)/2;
	t = time(nil);

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
	display->locking = 1;
	updateall(screen);
	unlockdisplay(display);

	while(recv(mc->c, &m) >= 0) {
		if (!(m.buttons & 1))
			continue;

		if (ptinrect(m.xy, buttons[0])) {
			t = time(nil);
			lockdisplay(display);
			updateall(screen);
			unlockdisplay(display);
			continue;
		}

		if (ptinrect(m.xy, buttons[1])) {
			t -= monthday * 24 * 3600;
			tm = localtime(t);
			if (monthday > monthlens[tm->mon])
				monthday = monthlens[tm->mon];
			t -= (monthlens[tm->mon] - monthday) * 24 * 3600;
			lockdisplay(display);
			updateall(screen);
			unlockdisplay(display);
			continue;
		}

		if (ptinrect(m.xy, buttons[2])) {
			tm = localtime(t);
			t += monthday * 24 * 3600;
			if (monthday > monthlens[tm->mon])
				monthday = monthlens[tm->mon];
			t += (monthlens[tm->mon] - monthday) * 24 * 3600;
			lockdisplay(display);
			updateall(screen);
			unlockdisplay(display);
			continue;
		}

		if (ptinrect(m.xy, buttons[3])) {
			t -= (isleap? 366: 365) * 24 * 3600;
			lockdisplay(display);
			updateall(screen);
			unlockdisplay(display);
			continue;
		}

		if (ptinrect(m.xy, buttons[4])) {
			t += (isleap? 366: 365) * 24 * 3600;
			lockdisplay(display);
			updateall(screen);
			unlockdisplay(display);
			continue;
		}

		for (y = 0; y < 6; y++) {
			for (x = 0; x < 7; x++) {
				if (mdays[x][y] == 0)
					continue;

				if (ptinrect(m.xy, days[x][y])) {
					t += (mdays[x][y] - monthday) * 24 * 3600;
					lockdisplay(display);
					updateall(screen);
					unlockdisplay(display);
				}
			}
		}

		half = Dx(screen->r)/2;
		textr = Rpt(Pt(screen->r.min.x + half, screen->r.min.y), screen->r.max);
		if (ptinrect(m.xy, textr))
			frselect(text, mc);
	}
}
