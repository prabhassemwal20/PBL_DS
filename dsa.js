/* ---------------- GEN-Scroll JavaScript (Simple YouTube Embed Version) ---------------- */

let feedContainer = document.getElementById("feed-container");
let currentIndex = 0;

// Only video IDs (no API flags)
let posts = [
  { type: "text", content: "Welcome to GEN-Scroll DSA Project!" },
  { type: "video", content: "xuP4g7IDgDM" },
  { type: "video", content: "-oOoTIuoL8M" },
  { type: "video", content: "278IRQ6HSi4" },
  { type: "video", content: "IxF55qB4CuQ" },
  { type: "video", content: "CsTQWvh-7A4" }
];

let pushCount = 0, popCount = 0, traverseCount = 0;
let autoTraverse;

/* ---------- Generate Clean YouTube Embed URL ---------- */
function getYouTubeEmbedURL(videoId) {
  return `https://www.youtube.com/embed/${videoId}`;
}

/* ---------- Display Current Post ---------- */
function showPost() {
  if (!feedContainer) return;
  if (posts.length === 0) {
    feedContainer.innerHTML = "<p>No posts available.</p>";
    return;
  }

  const post = posts[currentIndex];
  let html = "";

  if (post.type === "text") {
    html = `<div class="post"><p>${post.content}</p></div>`;
  } else if (post.type === "video") {
    const embed = getYouTubeEmbedURL(post.content);
    html = `
      <div class="video-wrapper">
        <iframe
          width="100%" height="100%"
          src="${embed}"
          frameborder="0"
          allow="accelerometer; clipboard-write; encrypted-media; gyroscope; picture-in-picture"
          allowfullscreen>
        </iframe>
      </div>`;
  }

  feedContainer.innerHTML = html;
  document.getElementById("push-count").innerText = pushCount;
  document.getElementById("pop-count").innerText = popCount;
  document.getElementById("traverse-count").innerText = traverseCount;
}

/* ---------- Stack Operations ---------- */
function pushPost() {
  const newVideo = { type: "video", content: "IxF55qB4CuQ" };
  posts.push(newVideo);
  pushCount++;
  currentIndex = posts.length - 1;
  showPost();
}

function popPost() {
  if (posts.length > 1) {
    posts.pop();
    popCount++;
    currentIndex = Math.max(0, posts.length - 1);
    showPost();
  }
}

function traversePost() {
  currentIndex = (currentIndex + 1) % posts.length;
  traverseCount++;
  showPost();
}

/* ---------- Auto Traverse Every 5 Seconds ---------- */
function startAutoTraverse() {
  if (autoTraverse) clearInterval(autoTraverse);
  autoTraverse = setInterval(() => {
    traversePost();
  }, 5000);
}

/* ---------- Init ---------- */
document.addEventListener("DOMContentLoaded", () => {
  showPost();
  startAutoTraverse();

  document.getElementById("push").addEventListener("click", pushPost);
  document.getElementById("pop").addEventListener("click", popPost);
  document.getElementById("traverse").addEventListener("click", traversePost);
});
