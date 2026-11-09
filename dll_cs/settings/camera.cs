// [camera]: how the view moves (engine/camera.c)
namespace ageofai.settings;

///
[section(20)]
class camera
{
    enum scroll_choice
    {
        /// the game's own: Empire Earth's constant speed, The Art of Conquest's acceleration
        game,
        /// Empire Earth's: constant speed (also in The Art of Conquest)
        ee,
        /// The Art of Conquest's: starts slowly and accelerates to the same top speed (also in Empire Earth)
        aoc,
    }
    /// Camera scrolling (mouse at the screen edge / scroll keys):
    [also("patches")]
    scroll_choice scroll = scroll_choice.game;
    /// Fine scrolling: the game moved the camera in fixed steps from a timer (every 10 or 25 ms) while frames come
    /// every few ms - some frames without a step, all steps the same size: jitter - and a busy game held the camera.
    /// 1 = the camera moves with every drawn frame by the time the frame took (same speed), also while the game is
    /// busy, 0 = the game's steps
    bool smooth_scroll = true;
    /// How far back the camera stands at the game's own furthest zoom, in tiles (0 = the game's 20.5). It levels out
    /// toward the ground on the way in as the game's camera does, at any value here:
    [range(0, 100)]
    int camera_distance = 25;
    /// Further out: past that point the wheel and the zoom keys go on to this many tiles back, the camera's angle
    /// staying as at the game's furthest zoom (0, or no more than camera_distance: no further; without the own renderer 42 at most):
    [range(0, 1000, error: true)]
    int camera_distance_max = 42;
    /// How far the camera sees, in tiles (0 = the game's 35); further out it grows with the camera's distance:
    [range(0, 100)]
    int camera_view_distance = 50;
    /// The field of view in degrees (0 = the game's 30):
    [range(0, 90)]
    int camera_fov = 48;
}
