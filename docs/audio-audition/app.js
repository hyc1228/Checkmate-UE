const STORAGE_KEY = "checkmate.audioAudition.v1";

const cues = [
  {
    key: "Ch1.Stamp",
    category: "Ch1",
    priority: "High",
    codeHook: true,
    placeholderReady: true,
    needsReplacement: true,
    fmodReady: false,
    file: "Ch1_Stamp.ogg",
    currentSource: "impactMetal_002",
    trigger: "DollDisplay::EnterConfirming / 印章砸下",
    target: "重金属盒盖啪 + 0.2s 齿轮咬合 tail",
    statusNote: "缺齿轮 tail"
  },
  {
    key: "Ch1.Toss",
    category: "Ch1",
    priority: "High",
    codeHook: true,
    placeholderReady: true,
    needsReplacement: true,
    fmodReady: false,
    file: "Ch1_Toss.ogg",
    currentSource: "thrusterFire_001",
    trigger: "DollDisplay::EnterTossing / 娃娃被扔",
    target: "咻 + 0.3s 落地砸开水面咚",
    statusNote: "缺落地和水面 tail"
  },
  {
    key: "Ch1.Correct",
    category: "Ch1",
    priority: "Medium",
    codeHook: true,
    placeholderReady: true,
    needsReplacement: false,
    fmodReady: false,
    file: "Ch1_Correct.ogg",
    currentSource: "engineCircular_000",
    trigger: "InspectionScreen::StartFeedback(true)",
    target: "齿轮咬上、机器接收信号的短确认",
    statusNote: "可用"
  },
  {
    key: "Ch1.Wrong",
    category: "Ch1",
    priority: "High",
    codeHook: true,
    placeholderReady: true,
    needsReplacement: true,
    fmodReady: false,
    file: "Ch1_Wrong.ogg",
    currentSource: "lowFrequency_explosion_000",
    trigger: "InspectionScreen::StartFeedback(false)",
    target: "低频闷响 + 哼鸣，工厂系统轻微抽搐",
    statusNote: "当前太爆炸感"
  },
  {
    key: "Ch1.ShiftPass",
    category: "Ch1",
    priority: "Medium",
    codeHook: true,
    placeholderReady: true,
    needsReplacement: true,
    fmodReady: false,
    file: "Ch1_ShiftPass.ogg",
    currentSource: "Kenney Digital / twoTone1",
    trigger: "Chapter1GameMode::HandleShiftCompleted(success)",
    target: "班次完成铃声 / 工厂下班音",
    statusNote: "电子占位"
  },
  {
    key: "Ch1.ShiftFail",
    category: "Ch1",
    priority: "Medium",
    codeHook: true,
    placeholderReady: true,
    needsReplacement: true,
    fmodReady: false,
    file: "Ch1_ShiftFail.ogg",
    currentSource: "Kenney Digital / lowDown",
    trigger: "Chapter1GameMode::HandleShiftCompleted(fail)",
    target: "班次失败 / 传送带停摆 / 系统下沉",
    statusNote: "电子占位"
  },
  {
    key: "Ch1.TimeoutWarn",
    category: "Ch1",
    priority: "Medium",
    codeHook: false,
    placeholderReady: true,
    needsReplacement: true,
    fmodReady: false,
    file: "Ch1_TimeoutWarn.ogg",
    currentSource: "Kenney Digital / highUp",
    trigger: "Shift 3/4 娃娃倒计时 < 3s",
    target: "滴答加快、紧迫提示",
    statusNote: "素材已放，触发点未接"
  },
  {
    key: "Ch1.Card.Select",
    category: "Ch1",
    priority: "Low",
    codeHook: false,
    placeholderReady: false,
    needsReplacement: false,
    fmodReady: false,
    file: "Ch1_Card_Select.ogg",
    currentSource: "",
    trigger: "玩家点选一张判据卡",
    target: "卡片浮起 + 浮光声效；比 UI.Click 更专属",
    statusNote: "待找"
  },
  {
    key: "Ch1.Card.Deselect",
    category: "Ch1",
    priority: "Low",
    codeHook: false,
    placeholderReady: false,
    needsReplacement: false,
    fmodReady: false,
    file: "Ch1_Card_Deselect.ogg",
    currentSource: "",
    trigger: "玩家反选一张判据卡",
    target: "卡片回落，短反气泡声",
    statusNote: "待找"
  },
  {
    key: "Ch1.ConveyorIn",
    category: "Ch1",
    priority: "High",
    codeHook: false,
    placeholderReady: false,
    needsReplacement: false,
    fmodReady: false,
    file: "Ch1_ConveyorIn.ogg",
    currentSource: "",
    trigger: "每只娃娃从左侧传送带滑入",
    target: "传送带咔啦启动 + 玩偶橡胶滑落",
    statusNote: "高优先级，未接触发点"
  },
  {
    key: "Ch1.DollSlide",
    category: "Ch1",
    priority: "Low",
    codeHook: false,
    placeholderReady: false,
    needsReplacement: false,
    fmodReady: false,
    file: "Ch1_DollSlide.ogg",
    currentSource: "",
    trigger: "DollDisplay 玩家拖动娃娃时",
    target: "娃娃在桌面上的橡胶摩擦 / 轻微布料拖动",
    statusNote: "低优先级，待找"
  },
  {
    key: "Ch2.Move",
    category: "Ch2",
    priority: "High",
    codeHook: true,
    placeholderReady: true,
    needsReplacement: true,
    fmodReady: false,
    file: "Ch2_Move.ogg",
    currentSource: "footstep_wood_001",
    trigger: "Ch2GameMode::NotifyMoveCommitted / 棋子落子",
    target: "木地板脚步 / 棋子落子；Ballet vs Clown 应不同",
    statusNote: "只有单一版本"
  },
  {
    key: "Ch2.Explode",
    category: "Ch2",
    priority: "High",
    codeHook: true,
    placeholderReady: true,
    needsReplacement: false,
    fmodReady: false,
    file: "Ch2_Explode.ogg",
    currentSource: "lowFrequency_explosion_001",
    trigger: "Ch2GameMode::ExplodePuppet",
    target: "闷沉爆炸 + 木裂，3D 衰减",
    statusNote: "可用"
  },
  {
    key: "Ch2.Ritual",
    category: "Ch2",
    priority: "High",
    codeHook: true,
    placeholderReady: true,
    needsReplacement: true,
    fmodReady: false,
    file: "Ch2_Ritual.ogg",
    currentSource: "doorOpen_000",
    trigger: "NotifyPawnEnteredCell(Pickup)",
    target: "取扣 / 缝扣，金属小门 + 钟铃单击，约 1.4s",
    statusNote: "当前太短"
  },
  {
    key: "Ch2.Victory",
    category: "Ch2",
    priority: "Medium",
    codeHook: true,
    placeholderReady: true,
    needsReplacement: false,
    fmodReady: false,
    file: "Ch2_Victory.ogg",
    currentSource: "confirmation_004",
    trigger: "NotifyPawnEnteredCell(Exit)",
    target: "长确认 + 钟铃 + 一丝色彩涌入",
    statusNote: "可用"
  },
  {
    key: "Ch2.Bell",
    category: "Ch2",
    priority: "High",
    codeHook: true,
    placeholderReady: true,
    needsReplacement: true,
    fmodReady: false,
    file: "Ch2_Bell.ogg",
    currentSource: "temporary duplicate of Ch2_Ritual",
    trigger: "Ch2GameMode::NotifyPawnEnteredCell / WeddingWreckage 4-neighbor",
    target: "SC_Ch2.Bell: <=600ms hard-cut short bell, no reverb tail",
    statusNote: "temporary placeholder"
  },
  {
    key: "Ch2.PuppetTick",
    category: "Ch2",
    priority: "Medium",
    codeHook: false,
    placeholderReady: true,
    needsReplacement: true,
    fmodReady: false,
    file: "Ch2_PuppetTick.ogg",
    currentSource: "Kenney Digital / tone1",
    trigger: "爆炸玩偶倒计时每回合脉冲",
    target: "心跳 / 滴答；玩家能听出还有 N 回合",
    statusNote: "素材已放，触发点未接"
  },
  {
    key: "Ch2.ModeBallet",
    category: "Ch2",
    priority: "Medium",
    codeHook: false,
    placeholderReady: false,
    needsReplacement: false,
    fmodReady: false,
    file: "Ch2_ModeBallet.ogg",
    currentSource: "",
    trigger: "Ch2.Move 的 Ballet 变种",
    target: "轻盈 + 滑步声",
    statusNote: "待找"
  },
  {
    key: "Ch2.ModeClown",
    category: "Ch2",
    priority: "Medium",
    codeHook: false,
    placeholderReady: false,
    needsReplacement: false,
    fmodReady: false,
    file: "Ch2_ModeClown.ogg",
    currentSource: "",
    trigger: "Ch2.Move 的 Clown 变种",
    target: "笨重 + 铃铛 / 双扣摇晃声",
    statusNote: "待找"
  },
  {
    key: "Ch2.Fail",
    category: "Ch2",
    priority: "Low",
    codeHook: false,
    placeholderReady: false,
    needsReplacement: false,
    fmodReady: false,
    file: "Ch2_Fail.ogg",
    currentSource: "",
    trigger: "Ch2 关卡步数超预算 / reset",
    target: "棋盘 reset，不复用 Ch1.Wrong",
    statusNote: "低优先级"
  },
  {
    key: "Twist.DroneAscent",
    category: "Twist",
    priority: "High",
    codeHook: true,
    placeholderReady: true,
    needsReplacement: true,
    fmodReady: false,
    file: "Twist_DroneAscent.ogg",
    currentSource: "Kenney Digital / lowThreeTone",
    trigger: "Chapter1GameMode::RequestTwist",
    target: "CameraReverse 低频上升，80Hz -> 160Hz，持续感",
    statusNote: "当前很短，仅占位"
  },
  {
    key: "Twist.PearlEye",
    category: "Twist",
    priority: "High",
    codeHook: true,
    placeholderReady: true,
    needsReplacement: true,
    fmodReady: false,
    file: "Twist_PearlEye.ogg",
    currentSource: "Kenney Digital / spaceTrash1",
    trigger: "Chapter1GameMode::RequestTwist / 按扣脱落",
    target: "短促啪嗒 + 线断 / 玻璃裂，物理感多于电子感",
    statusNote: "电子感偏强"
  },
  {
    key: "Twist.MechanicalEye",
    category: "Twist",
    priority: "High",
    codeHook: true,
    placeholderReady: true,
    needsReplacement: true,
    fmodReady: false,
    file: "Twist_MechanicalEye.ogg",
    currentSource: "Kenney Digital / phaserUp4",
    trigger: "Chapter1GameMode::PlayTwistOpticalBurnout",
    target: "机械眼启动 / bloom pulse，电子 + 钟铃混合",
    statusNote: "第一版可用"
  },
  {
    key: "UI.Click",
    category: "UI",
    priority: "Medium",
    codeHook: true,
    placeholderReady: true,
    needsReplacement: false,
    fmodReady: false,
    file: "UI_Click.ogg",
    currentSource: "click_001",
    trigger: "MainMenu / JudgmentCard / CardSelection click",
    target: "UI 按钮 / 卡片点击的轻确认",
    statusNote: "可用"
  },
  {
    key: "UI.Hover",
    category: "UI",
    priority: "Low",
    codeHook: true,
    placeholderReady: true,
    needsReplacement: false,
    fmodReady: false,
    file: "UI_Hover.ogg",
    currentSource: "click_005",
    trigger: "MainMenu hover",
    target: "极轻 tick",
    statusNote: "可用"
  },
  {
    key: "UI.LangToggle",
    category: "UI",
    priority: "Low",
    codeHook: false,
    placeholderReady: false,
    needsReplacement: false,
    fmodReady: false,
    file: "UI_LangToggle.ogg",
    currentSource: "",
    trigger: "中英切换按钮",
    target: "短 click，可复用 UI.Click",
    statusNote: "可省"
  },
  {
    key: "Amb.Ch1",
    category: "Ambient",
    priority: "High",
    codeHook: true,
    placeholderReady: true,
    needsReplacement: true,
    fmodReady: false,
    file: "Amb_Ch1.ogg",
    currentSource: "spaceEngineLow_000",
    trigger: "Chapter1GameMode::BeginPlay",
    target: "30s+ seamless 工厂底噪：传送带 + 机器嗡鸣 + 偶发金属",
    statusNote: "5s 短样本，有接缝"
  },
  {
    key: "Amb.Ch2",
    category: "Ambient",
    priority: "High",
    codeHook: true,
    placeholderReady: true,
    needsReplacement: true,
    fmodReady: false,
    file: "Amb_Ch2.ogg",
    currentSource: "spaceEngineSmall_000",
    trigger: "Ch2GameMode::BeginPlay",
    target: "30s+ seamless 空厂 ambient：风、滴水、远处金属",
    statusNote: "5s 短样本，有接缝"
  },
  {
    key: "Music.MainMenu",
    category: "Music",
    priority: "High",
    codeHook: false,
    placeholderReady: false,
    needsReplacement: false,
    fmodReady: false,
    file: "Music_MainMenu.ogg",
    currentSource: "",
    trigger: "主菜单 BGM loop",
    target: "长 ambient 主题音乐，定调全游戏氛围",
    statusNote: "待找"
  },
  {
    key: "Music.Ch1Loop",
    category: "Music",
    priority: "Medium",
    codeHook: false,
    placeholderReady: false,
    needsReplacement: false,
    fmodReady: false,
    file: "Music_Ch1Loop.ogg",
    currentSource: "",
    trigger: "Ch1 班次中循环",
    target: "低频脉动节奏 + 偶发齿轮 motif",
    statusNote: "待找"
  },
  {
    key: "Music.Ch2Loop",
    category: "Music",
    priority: "Medium",
    codeHook: false,
    placeholderReady: false,
    needsReplacement: false,
    fmodReady: false,
    file: "Music_Ch2Loop.ogg",
    currentSource: "",
    trigger: "Ch2 棋盘逃脱中循环",
    target: "稀疏、冷的 piano + drone",
    statusNote: "待找"
  }
];

const fmodEvents = {
  "Ch1.Stamp": "event:/Ch1/Stamp",
  "Ch1.Toss": "event:/Ch1/Toss",
  "Ch1.Correct": "event:/Ch1/Correct",
  "Ch1.Wrong": "event:/Ch1/Wrong",
  "Ch1.ShiftPass": "event:/Ch1/ShiftPass",
  "Ch1.ShiftFail": "event:/Ch1/ShiftFail",
  "Ch1.TimeoutWarn": "event:/Ch1/TimeoutWarn",
  "Ch1.Card.Select": "event:/Ch1/CardSelect",
  "Ch1.Card.Deselect": "event:/Ch1/CardDeselect",
  "Ch1.ConveyorIn": "event:/Ch1/ConveyorIn",
  "Ch1.DollSlide": "event:/Ch1/DollSlide",
  "Ch2.Move": "event:/Ch2/Move",
  "Ch2.Explode": "event:/Ch2/Explode",
  "Ch2.Ritual": "event:/Ch2/Ritual",
  "Ch2.Bell": "event:/Ch2/Bell",
  "Ch2.Victory": "event:/Ch2/Victory",
  "Ch2.PuppetTick": "event:/Ch2/PuppetTick",
  "Ch2.ModeBallet": "event:/Ch2/Move",
  "Ch2.ModeClown": "event:/Ch2/Move",
  "Ch2.Fail": "event:/Ch2/Fail",
  "Twist.DroneAscent": "event:/Twist/DroneAscent",
  "Twist.PearlEye": "event:/Twist/PearlEye",
  "Twist.MechanicalEye": "event:/Twist/MechanicalEye",
  "UI.Click": "event:/UI/Click",
  "UI.Hover": "event:/UI/Hover",
  "UI.LangToggle": "event:/UI/LangToggle",
  "Amb.Ch1": "event:/Amb/Ch1Factory",
  "Amb.Ch2": "event:/Amb/Ch2Empty",
  "Music.MainMenu": "event:/Music/MainMenu",
  "Music.Ch1Loop": "event:/Music/Ch1Loop",
  "Music.Ch2Loop": "event:/Music/Ch2Loop"
};

const state = loadState();
const cueList = document.querySelector("#cueList");
const template = document.querySelector("#cueTemplate");
const searchInput = document.querySelector("#searchInput");
const categoryFilter = document.querySelector("#categoryFilter");
const statusFilter = document.querySelector("#statusFilter");
const decisionFilter = document.querySelector("#decisionFilter");
const activeObjectUrls = new Map();

function loadState() {
  try {
    return JSON.parse(localStorage.getItem(STORAGE_KEY)) || {};
  } catch {
    return {};
  }
}

function saveState() {
  localStorage.setItem(STORAGE_KEY, JSON.stringify(state));
  updateMetrics();
}

function cueState(key) {
  if (!state[key]) {
    state[key] = {
      decision: "untested",
      replacement: "",
      notes: ""
    };
  }
  return state[key];
}

function placeholderPath(cue) {
  return cue.placeholderReady ? `../../Content/Audio/Placeholder/${cue.file}` : "";
}

function fmodPath(cue) {
  return fmodEvents[cue.key] || `event:/Todo/${cue.key.replaceAll(".", "/")}`;
}

function statusBadges(cue, review) {
  const badges = [];
  badges.push({ text: cue.category, type: "info" });
  badges.push({ text: cue.priority, type: cue.priority === "High" ? "warn" : "info" });
  badges.push({ text: cue.codeHook ? "Code Hooked" : "No Hook", type: cue.codeHook ? "good" : "bad" });
  badges.push({ text: cue.placeholderReady ? "Placeholder" : "No Audio", type: cue.placeholderReady ? "good" : "bad" });
  badges.push({ text: cue.needsReplacement ? "Needs Replace" : "Usable", type: cue.needsReplacement ? "warn" : "good" });
  badges.push({ text: cue.fmodReady ? "FMOD Ready" : "FMOD Todo", type: cue.fmodReady ? "good" : "warn" });
  if (review.decision !== "untested") {
    badges.push({ text: review.decision, type: review.decision === "approved" ? "good" : review.decision === "reject" ? "bad" : "warn" });
  }
  return badges;
}

function renderCategoryOptions() {
  const categories = [...new Set(cues.map(cue => cue.category))].sort();
  for (const category of categories) {
    const option = document.createElement("option");
    option.value = category;
    option.textContent = category;
    categoryFilter.appendChild(option);
  }
}

function matchesFilters(cue) {
  const review = cueState(cue.key);
  const haystack = [
    cue.key,
    cue.category,
    cue.trigger,
    cue.target,
    cue.file,
    cue.currentSource,
    cue.statusNote,
    review.replacement,
    review.notes
  ].join(" ").toLowerCase();
  const search = searchInput.value.trim().toLowerCase();
  if (search && !haystack.includes(search)) return false;
  if (categoryFilter.value !== "all" && cue.category !== categoryFilter.value) return false;
  if (decisionFilter.value !== "all" && review.decision !== decisionFilter.value) return false;

  switch (statusFilter.value) {
    case "hooked": return cue.codeHook;
    case "not-hooked": return !cue.codeHook;
    case "has-placeholder": return cue.placeholderReady;
    case "missing-placeholder": return !cue.placeholderReady;
    case "needs-replacement": return cue.needsReplacement;
    case "approved": return review.decision === "approved";
    default: return true;
  }
}

function render() {
  cueList.innerHTML = "";
  const visible = cues.filter(matchesFilters);

  for (const cue of visible) {
    const review = cueState(cue.key);
    const node = template.content.firstElementChild.cloneNode(true);

    node.dataset.key = cue.key;
    node.querySelector(".cueKey").textContent = cue.key;
    node.querySelector(".cueMeta").textContent = `${cue.category} / ${cue.priority} priority / target file: ${cue.file}`;
    node.querySelector(".trigger").textContent = cue.trigger;
    node.querySelector(".target").textContent = cue.target;
    node.querySelector(".placeholder").textContent = cue.placeholderReady
      ? `${cue.file} (${cue.currentSource || "source unknown"}) - ${cue.statusNote}`
      : `Missing placeholder. Target filename: ${cue.file}`;
    node.querySelector(".fmod").textContent = cue.fmodReady ? "Ready" : `${fmodPath(cue)} (todo)`;

    const badges = node.querySelector(".badges");
    for (const badge of statusBadges(cue, review)) {
      const span = document.createElement("span");
      span.className = `badge ${badge.type}`;
      span.textContent = badge.text;
      badges.appendChild(span);
    }

    const currentAudio = node.querySelector(".currentAudio");
    if (cue.placeholderReady) {
      currentAudio.src = placeholderPath(cue);
    } else {
      currentAudio.classList.add("hidden");
    }

    const candidateInput = node.querySelector(".candidateInput");
    const candidateAudio = node.querySelector(".candidateAudio");
    const candidateName = node.querySelector(".candidateName");
    candidateAudio.classList.add("hidden");
    candidateInput.addEventListener("change", event => {
      const file = event.target.files?.[0];
      if (!file) return;
      if (activeObjectUrls.has(cue.key)) {
        URL.revokeObjectURL(activeObjectUrls.get(cue.key));
      }
      const url = URL.createObjectURL(file);
      activeObjectUrls.set(cue.key, url);
      candidateAudio.src = url;
      candidateAudio.classList.remove("hidden");
      candidateName.textContent = `${file.name} -> replace as ${cue.file}`;
      review.replacement = file.name;
      node.querySelector(".replacementInput").value = review.replacement;
      saveState();
    });

    const decisionSelect = node.querySelector(".decisionSelect");
    decisionSelect.value = review.decision;
    decisionSelect.addEventListener("change", () => {
      review.decision = decisionSelect.value;
      saveState();
      render();
    });

    const replacementInput = node.querySelector(".replacementInput");
    replacementInput.value = review.replacement || "";
    replacementInput.addEventListener("input", () => {
      review.replacement = replacementInput.value;
      saveState();
    });

    const notesInput = node.querySelector(".notesInput");
    notesInput.value = review.notes || "";
    notesInput.addEventListener("input", () => {
      review.notes = notesInput.value;
      saveState();
    });

    cueList.appendChild(node);
  }

  updateMetrics();
}

function updateMetrics() {
  const reviews = cues.map(cue => cueState(cue.key));
  document.querySelector("#metricTotal").textContent = cues.length;
  document.querySelector("#metricHooked").textContent = cues.filter(cue => cue.codeHook).length;
  document.querySelector("#metricPlaceholder").textContent = cues.filter(cue => cue.placeholderReady).length;
  document.querySelector("#metricNeeds").textContent = cues.filter(cue => cue.needsReplacement).length;
  document.querySelector("#metricApproved").textContent = reviews.filter(review => review.decision === "approved").length;
}

function stopAllAudio() {
  document.querySelectorAll("audio").forEach(audio => {
    audio.pause();
    audio.currentTime = 0;
  });
}

async function playVisibleSequentially() {
  stopAllAudio();
  const players = [...document.querySelectorAll(".cueCard:not(.hidden) .currentAudio")]
    .filter(audio => audio.src);
  for (const audio of players) {
    await new Promise(resolve => {
      const done = () => {
        audio.removeEventListener("ended", done);
        audio.removeEventListener("error", done);
        resolve();
      };
      audio.addEventListener("ended", done);
      audio.addEventListener("error", done);
      audio.currentTime = 0;
      audio.play().catch(done);
    });
  }
}

function exportReviews() {
  const payload = {
    exportedAt: new Date().toISOString(),
    replacementRoot: "Content/Audio/Placeholder",
    cues: cues.map(cue => ({
      key: cue.key,
      category: cue.category,
      codeHook: cue.codeHook,
      placeholderReady: cue.placeholderReady,
      targetFile: cue.file,
      designTarget: cue.target,
      currentSource: cue.currentSource,
      fmodEvent: fmodPath(cue),
      review: cueState(cue.key)
    }))
  };
  const blob = new Blob([JSON.stringify(payload, null, 2)], { type: "application/json" });
  const link = document.createElement("a");
  link.href = URL.createObjectURL(blob);
  link.download = `checkmate-audio-review-${new Date().toISOString().slice(0, 10)}.json`;
  link.click();
  URL.revokeObjectURL(link.href);
}

function importReviews(file) {
  const reader = new FileReader();
  reader.onload = () => {
    try {
      const payload = JSON.parse(reader.result);
      const imported = Array.isArray(payload.cues) ? payload.cues : [];
      for (const item of imported) {
        if (item.key && item.review) {
          state[item.key] = {
            decision: item.review.decision || "untested",
            replacement: item.review.replacement || "",
            notes: item.review.notes || ""
          };
        }
      }
      saveState();
      render();
    } catch (error) {
      alert(`Import failed: ${error.message}`);
    }
  };
  reader.readAsText(file);
}

renderCategoryOptions();
render();

searchInput.addEventListener("input", render);
categoryFilter.addEventListener("change", render);
statusFilter.addEventListener("change", render);
decisionFilter.addEventListener("change", render);
document.querySelector("#stopBtn").addEventListener("click", stopAllAudio);
document.querySelector("#playVisibleBtn").addEventListener("click", playVisibleSequentially);
document.querySelector("#exportBtn").addEventListener("click", exportReviews);
document.querySelector("#importInput").addEventListener("change", event => {
  const file = event.target.files?.[0];
  if (file) importReviews(file);
});
