#include "rl_utils.hpp"

#include <algorithm>
#include <vector>
#include <cassert>
#include <sstream>

#include "mersenne_twister.hpp"

namespace do_not_call
{

void assert_impl(const bool check,
                 const char* check_str,
                 const char* file,
                 const int line,
                 const char* func)
{
    if (!check)
    {
        std::cerr << std::endl
                  << file << ", "
                  << line << ", "
                  << func << "():"
                  << std::endl
                  << std::endl << "*** ASSERTION FAILED! ***"
                  << std::endl
                  << std::endl << check_str
                  << std::endl
                  << std::endl;

        assert(false);
    }
}

}

namespace dir_utils
{

namespace
{

const std::string compass_dir_names[3][3] =
{
    {"NW", "N", "NE"},
    {"W",  "",  "E",},
    {"SW", "S", "SE"}
};

const double pi_db            = 3.14159265;
const double angle_45_db      = 2 * pi_db / 8;
const double angle_45_half_db = angle_45_db / 2.0;

const double edge[4] =
{
    angle_45_half_db + (angle_45_db * 0),
    angle_45_half_db + (angle_45_db * 1),
    angle_45_half_db + (angle_45_db * 2),
    angle_45_half_db + (angle_45_db * 3)
};

} //namespace

Dir dir(const P& offset)
{
    ASSERT(offset.x >= -1 &&
           offset.y >= -1 &&
           offset.x <=  1 &&
           offset.y <=  1);

    if (offset.y == -1)
    {
        return offset.x == -1 ? Dir::up_left :
               offset.x ==  0 ? Dir::up :
               offset.x ==  1 ? Dir::up_right :
               Dir::END;
    }

    if (offset.y == 0)
    {
        return offset.x == -1 ? Dir::left :
               offset.x ==  0 ? Dir::center :
               offset.x ==  1 ? Dir::right :
               Dir::END;
    }

    if (offset.y == 1)
    {
        return offset.x == -1 ? Dir::down_left :
               offset.x ==  0 ? Dir::down :
               offset.x ==  1 ? Dir::down_right :
               Dir::END;
    }

    return Dir::END;
}

P offset(const Dir dir)
{
    ASSERT(dir != Dir::END);

    switch (dir)
    {
    case Dir::down_left:
        return P(-1, 1);

    case Dir::down:
        return P(0, 1);

    case Dir::down_right:
        return P(1, 1);

    case Dir::left:
        return P(-1, 0);

    case Dir::center:
        return P(0, 0);

    case Dir::right:
        return P(1, 0);

    case Dir::up_left:
        return P(-1, -1);

    case Dir::up:
        return P(0, -1);

    case Dir::up_right:
        return P(1, -1);

    case Dir::END:
        return P(0, 0);
    }

    return P(0, 0);
}

P rnd_adj_pos(const P& origin, const bool is_center_allowed)
{
    const std::vector<P>* vec = nullptr;

    if (is_center_allowed)
    {
        vec = &dir_list_w_center;
    }
    else //Center not allowed
    {
        vec = &dir_list;
    }

    const size_t idx = rnd::range(0, vec->size() - 1);

    return origin + vec->at(idx);
}

void compass_dir_name(const P& from_pos,
                      const P& to_pos,
                      std::string& dst)
{
    dst = "";

    const P offset(to_pos - from_pos);
    const double angle_db = atan2(-offset.y, offset.x);

    if (angle_db        <  -edge[2] && angle_db >  -edge[3])
    {
        dst = "SW";
    }
    else if (angle_db <= -edge[1] && angle_db >= -edge[2])
    {
        dst = "S";
    }
    else if (angle_db <  -edge[0] && angle_db >  -edge[1])
    {
        dst = "SE";
    }
    else if (angle_db >= -edge[0] && angle_db <=  edge[0])
    {
        dst = "E";
    }
    else if (angle_db >   edge[0] && angle_db <   edge[1])
    {
        dst = "NE";
    }
    else if (angle_db >=  edge[1] && angle_db <=  edge[2])
    {
        dst = "N";
    }
    else if (angle_db >   edge[2] && angle_db <   edge[3])
    {
        dst = "NW";
    }
    else
    {
        dst = "W";
    }
}

void compass_dir_name(const Dir dir, std::string& dst)
{

    const P& o  = offset(dir);
    dst         = compass_dir_names[o.x + 1][o.y + 1];
}

void compass_dir_name(const P& offs, std::string& dst)
{
    dst = compass_dir_names[offs.x + 1][offs.y + 1];
}

} //dir_utils

int Dice_param::roll() const
{
    return rnd::dice(rolls, sides) + plus;
}

int Range::roll() const
{
    return rnd::range(min, max);
}

bool Fraction::roll() const
{
    return rnd::fraction(num, den);
}

namespace rnd
{

namespace
{

MTRand mt_rand;

int roll(const int rolls, const int sides)
{
    if (sides <= 0)
    {
        return 0;
    }

    if (sides == 1)
    {
        return rolls * sides;
    }

    int result = 0;

    for (int i = 0; i < rolls; ++i)
    {
        result += mt_rand.randInt(sides - 1) + 1;
    }

    return result;
}

} //namespace

void seed(const unsigned long val)
{
    mt_rand = MTRand(val);
}

int dice(const int rolls, const int sides)
{
    return roll(rolls, sides);
}

bool coin_toss()
{
    return roll(1, 2) == 2;
}

bool fraction(const int numer, const int denom)
{
    //This function should never be called with a denominator less than one, since it's unclear
    //what it means that something should happen e.g. "N times in 0", or "N times in -1".
    ASSERT(denom >= 1);

    //If numerator is bigger than denominator, it's most likely a bug (should something occur e.g.
    //5 times in 3 ???) - don't allow this...
    ASSERT(numer <= denom);

    //A negative numerator is of course nonsense
    ASSERT(numer >= 0);

    //If any of the rules above are broken on a release build, try to perform the action that was
    //*probably* intended.

    //NOTE: A numerator of 0 is always allowed (it simply means "no chance")

    if ((numer <= 0) || (denom <= 0))
    {
        return false;
    }

    if ((numer >= denom) || (denom == 1))
    {
        return true;
    }

    return roll(1, denom) <= numer;
}

bool one_in(const int N)
{
    return fraction(1, N);
}

int range(const int v1, const int v2)
{
    const int min = std::min(v1, v2);
    const int max = std::max(v1, v2);

    return min + roll(1, max - min + 1) - 1;
}

int percent()
{
    return roll(1, 100);
}

bool percent(const int pct_chance)
{
    return pct_chance >= roll(1, 100);
}

int weighted_choice(const std::vector<int> weights)
{
#ifndef NDEBUG
    for (const int weight : weights)
    {
        ASSERT(weight > 0);
    }
#endif // NDEBUG

    const int sum = std::accumulate(begin(weights), end(weights), 0);

    int rnd = rnd::range(0, sum - 1);

    for (size_t i = 0; i < weights.size(); ++i)
    {
        const int weight = weights[i];

        if (rnd < weight)
        {
            return i;
        }

        rnd -= weight;
    }

    //This point should never be reached
    ASSERT(false);

    return 0;
}

} //rnd

void set_constr_in_range(const int min, int& val, const int max)
{
    if (max >= min)
    {
        val = std::min(max, std::max(val, min));
    }
}

void set_constr_in_range(const double min, double& val, const double max)
{
    if (max > min)
    {
        val = std::min(max, std::max(val, min));
    }
}

int constr_in_range(const int min, const int val, const int max)
{
    if (max < min)
    {
        return -1;
    }

    return std::min(max, std::max(val, min));
}

int constr_in_range(const double min, const double val, const double max)
{
    if (max < min)
    {
        return -1;
    }

    return std::min(max, std::max(val, min));
}

void to_vec(const bool* array2,
            const bool value_to_store,
            const int w,
            const int h,
            std::vector<P>& out)
{
    out.clear();

    for (int x = 0; x < w; ++x)
    {
        for (int y = 0; y < h; ++y)
        {
            const bool v = *(array2 + (x * h) + y);

            if (v == value_to_store)
            {
                out.push_back(P(x, y));
            }
        }
    }
}
bool is_pos_inside(const P& pos, const R& area)
{
    return
        pos.x >= area.p0.x &&
        pos.x <= area.p1.x &&
        pos.y >= area.p0.y &&
        pos.y <= area.p1.y;
}

bool is_area_inside(const R& inner,
                    const R& outer,
                    const bool count_equal_as_inside)
{
    if (count_equal_as_inside)
    {
        return inner.p0.x >= outer.p0.x &&
               inner.p1.x <= outer.p1.x &&
               inner.p0.y >= outer.p0.y &&
               inner.p1.y <= outer.p1.y;
    }
    else
    {
        return inner.p0.x > outer.p0.x &&
               inner.p1.x < outer.p1.x &&
               inner.p0.y > outer.p0.y &&
               inner.p1.y < outer.p1.y;
    }
}

int king_dist(const int x0, const int y0, const int x1, const int y1)
{
    return std::max(abs(x1 - x0), abs(y1 - y0));
}

int king_dist(const P& p0, const P& p1)
{
    return std::max(abs(p1.x - p0.x), abs(p1.y - p0.y));
}

int taxi_dist(const P& p0, const P& p1)
{
    return abs(p1.x - p0.x) + abs(p1.y - p0.y);
}

P closest_pos(const P& p, const std::vector<P>& positions)
{
    int dist_to_nearest = INT_MAX;

    P closest_pos;

    for (P p_cmp : positions)
    {
        const int cur_dist = king_dist(p, p_cmp);

        if (cur_dist < dist_to_nearest)
        {
            dist_to_nearest = cur_dist;
            closest_pos     = p_cmp;
        }
    }

    return closest_pos;
}

bool is_pos_adj(const P& pos1,
                const P& pos2,
                const bool count_same_cell_as_adj)
{
    if (
        pos1.x < pos2.x - 1 ||
        pos1.x > pos2.x + 1 ||
        pos1.y < pos2.y - 1 ||
        pos1.y > pos2.y + 1)
    {
        return false;
    }
    else if (pos1.x == pos2.x && pos1.y == pos2.y)
    {
        return count_same_cell_as_adj;
    }

    return true;
}

bool is_val_in_range(const int V, const Range range)
{
    return range.is_in_range(V);
}

Time_data cur_time()
{
    time_t t        = time(nullptr);
    struct tm* now  = localtime(&t);

    return Time_data(now->tm_year + 1900,
                     now->tm_mon + 1,
                     now->tm_mday,
                     now->tm_hour,
                     now->tm_min,
                     now->tm_sec);
}

std::string Time_data::time_str(const Time_type lowest, const bool add_separators) const
{
    std::string ret = to_str(year_);

    const std::string month_str   = (month_   < 10 ? "0" : "") + to_str(month_);
    const std::string day_str     = (day_     < 10 ? "0" : "") + to_str(day_);
    const std::string hour_str    = (hour_    < 10 ? "0" : "") + to_str(hour_);
    const std::string minute_str  = (minute_  < 10 ? "0" : "") + to_str(minute_);
    const std::string second_str  = (second_  < 10 ? "0" : "") + to_str(second_);

    if (lowest >= Time_type::month)
    {
        ret += "-" + month_str;
    }

    if (lowest >= Time_type::day)
    {
        ret += "-" + day_str;
    }

    if (lowest >= Time_type::hour)
    {
        ret += (add_separators ? " " : "_") + hour_str;
    }

    if (lowest >= Time_type::minute)
    {
        ret += (add_separators ? ":" : "-") + minute_str;
    }

    if (lowest >= Time_type::second)
    {
        ret += (add_separators ? ":" : "-") + second_str;
    }

    return ret;
}

std::string to_str(const int in)
{
    std::ostringstream buffer;
    buffer << in;
    return buffer.str();
}

int to_int(const std::string& in)
{
    int _nr;
    std::istringstream buffer(in);
    buffer >> _nr;
    return _nr;
}
