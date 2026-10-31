// [camera]: how the view moves (engine/camera.c)
namespace ageofai.settings;

///
[section(20)]
class camera
{
    enum scroll_choice { game, ee, aoc }

    /// Camera scrolling (mouse at the screen edge / scroll keys):
    ///   game = the game's own: Empire Earth's constant speed, The Art of Conquest's acceleration
    ///   ee   = Empire Earth's: constant speed (also in The Art of Conquest)
    ///   aoc  = The Art of Conquest's: starts slowly and accelerates to the same top speed (also in Empire Earth)
    scroll_choice scroll = scroll_choice.game;
    /// Fine scrolling: the game moved the camera in fixed steps from a timer (every 10 or 25 ms) while frames come
    /// every few ms - some frames without a step, all steps the same size: jitter - and a busy game held the camera.
    /// 1 = the camera moves with every drawn frame by the time the frame took (same speed), also while the game is
    /// busy, 0 = the game's steps
    bool smooth_scroll = true;
}
