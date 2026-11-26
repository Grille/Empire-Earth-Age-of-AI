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
    /// Earthquake shake: the game shook the screen in an earthquake by moving the camera itself a little every step,
    /// never back - it wandered off, and fought the fine scrolling and the zoom. 1 = the view shakes around where the
    /// camera stands, the camera stays put (with the mod's renderer), 0 = the game's
    bool quake_shake = true;
    /// How far back the camera stands at the game's own furthest zoom, in tiles (0 = the game's 20.5). It levels out
    /// toward the ground on the way in as the game's camera does, at any value here:
    [range(0, 100)]
    int camera_distance = 25;
    /// Further out: past that point the wheel and the zoom keys go on to this many tiles back, the camera's angle
    /// staying as at the game's furthest zoom (0, or no more than camera_distance: no further; without the own renderer 42 at most):
    [range(0, 1000, error: true)]
    int camera_distance_max = 100;
    /// 1 = the mouse wheel zooms at the mouse, the ground under the cursor stays under it: in full while the cursor looks
    /// 15 degrees or more down at the ground, fading out nearer the horizon (near in the camera tilts toward it). 0 = at the middle of the screen
    bool camera_zoom_at_mouse = true;
    /// Free turn in play as in the scenario editor: 1 = the ^ key (` on an English keyboard) held turns the camera with the
    /// mouse, the view then the isometric one (it keeps its angle when scrolling and zooming; back to the normal view:
    /// F2, the perspective key). 0 = only in the scenario editor, as in the game
    bool camera_free_turn = true;
    /// The epic view (F2 once): the game's turns it 12 degrees further down at the last notch before the furthest zoom,
    /// to the normal view's 35. 1 = its angle runs on evenly and ends at the one a notch before (23 degrees down), further
    /// out it stays at that. 0 = the game's
    bool camera_epic_angle = true;
    /// How far the camera sees, in tiles (0 = the game's 35); further out it grows with the camera's distance:
    [range(0, 100)]
    int camera_view_distance = 100;
    /// The own renderer: where the distance's fog starts, as a share of how far the camera sees at that zoom (the fog's
    /// end, which grows as the camera goes out). 0% = the game's own: a band of 8 tiles before the end - with a long view
    /// distance the far land stays clear up to there and then fades within those tiles. Lower starts the haze nearer,
    /// over more of the way (the control pipe: picture fog <0..0.95>):
    [advanced, range(0, 0.95)]
    share fog_start = 0.5;
    /// The field of view in degrees (0 = the game's 30):
    [range(0, 90)]
    int camera_fov = 50;
}
