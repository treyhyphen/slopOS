#!/usr/bin/env python3
"""
Parse Copilot chat JSON files and render them as markdown.
Extracts user prompts and responses with timestamps, deduplicating identical entries.
"""

import json
import os
import re
from datetime import datetime, timezone
from zoneinfo import ZoneInfo
from pathlib import Path
from typing import List, Dict, Set, Tuple


def load_chat_file(filepath: str) -> Dict:
    """Load and parse a JSON chat file."""
    with open(filepath, 'r', encoding='utf-8') as f:
        return json.load(f)


def extract_text_from_parts(parts: List) -> str:
    """Extract text from message parts."""
    if not parts:
        return ""
    
    text_parts = []
    for part in parts:
        if isinstance(part, dict) and 'text' in part:
            text_parts.append(part['text'])
        elif isinstance(part, str):
            text_parts.append(part)
    
    return " ".join(text_parts).strip()


def extract_response_text(response: List) -> str:
    """Extract text from response array."""
    text_parts = []
    for item in response:
        if isinstance(item, dict):
            # Skip items that are just URIs or other metadata
            kind = item.get('kind', '')
            if kind in ('codeblockUri', 'uri', 'inlineReference'):
                continue
                
            if 'value' in item and isinstance(item['value'], str):
                text_parts.append(item['value'])
            elif 'text' in item:
                text_parts.append(item['text'])
    
    # Concatenate all parts - don't add spaces between them as they might be code blocks
    full_text = "".join(text_parts).strip()
    
    # If response is incomplete/truncated, indicate this
    if full_text and not full_text.endswith(('.', '!', '?', '```', '````', '\n')):
        # Response seems truncated, add note
        full_text += "\n\n*[Response was incomplete/canceled]*"
    
    return full_text


def format_timestamp(timestamp_ms: int) -> str:
    """Convert millisecond timestamp to readable format in US/Eastern timezone."""
    # Convert from UTC to US/Eastern
    dt_utc = datetime.fromtimestamp(timestamp_ms / 1000, tz=timezone.utc)
    eastern = ZoneInfo('America/New_York')
    dt_eastern = dt_utc.astimezone(eastern)
    return dt_eastern.strftime("%Y-%m-%d %H:%M:%S %Z")


def categorize_prompt(text: str) -> str:
    """
    Categorize a user prompt based on its content.
    Returns a category tag like "Feature Request", "Bug Fix", etc.
    """
    text_lower = text.lower()
    
    # Question patterns
    question_patterns = [
        r'\bhow (do|can|to|does|did|would|should)',
        r'\bwhat (is|are|does|do|was|were)',
        r'\bwhy (is|are|does|do|did)',
        r'\bwhere (is|are|does|do)',
        r'\bwhen (is|are|does|do|should)',
        r'\bcan (you|i|we)',
        r'\?',
    ]
    
    # Feature/enhancement patterns
    feature_patterns = [
        r'\badd (a|an|support|networking|feature)',
        r'\bcreate (a|an|new)',
        r'\bimplement',
        r'\bbuild',
        r'\bset up',
        r'\bsetup',
        r'\bgenerate',
        r'\bmake (a|an|it)',
        r'\bnew (feature|functionality)',
        r'\benhance',
        r'\bextend',
    ]
    
    # Bug fix patterns
    bug_patterns = [
        r'\bfix',
        r'\bbug',
        r'\berror',
        r'\bissue',
        r'\bproblem',
        r'\bfail(ed|ing|ure)',
        r'\bbroken',
        r'\bnot working',
        r'\bdoesn\'t work',
        r'\bcrash',
    ]
    
    # Refactoring/modification patterns
    refactor_patterns = [
        r'\brefactor',
        r'\bmodify',
        r'\bchange',
        r'\bupdate',
        r'\bedit',
        r'\bimprove',
        r'\boptimize',
        r'\breorganize',
        r'\brestructure',
        r'\bclean up',
    ]
    
    # Documentation patterns
    doc_patterns = [
        r'\bdocument',
        r'\bwrite (a|an|the) (readme|guide|doc)',
        r'\bexplain',
        r'\bdescribe',
    ]
    
    # Testing patterns
    test_patterns = [
        r'\btest',
        r'\brun.*test',
        r'\bdebug',
        r'\bvalidate',
    ]
    
    # Configuration patterns
    config_patterns = [
        r'\bconfigure',
        r'\bconfig',
        r'\bsetup.*environment',
        r'\binstall',
        r'\bgit',
        r'\bgithub actions',
        r'\bci/cd',
    ]
    
    # Check patterns in priority order
    if any(re.search(pattern, text_lower) for pattern in bug_patterns):
        return "🐛 Bug Fix"
    
    if any(re.search(pattern, text_lower) for pattern in test_patterns):
        return "🧪 Testing"
    
    if any(re.search(pattern, text_lower) for pattern in feature_patterns):
        return "✨ Feature Request"
    
    if any(re.search(pattern, text_lower) for pattern in refactor_patterns):
        return "♻️ Refactoring"
    
    if any(re.search(pattern, text_lower) for pattern in config_patterns):
        return "⚙️ Configuration"
    
    if any(re.search(pattern, text_lower) for pattern in doc_patterns):
        return "📝 Documentation"
    
    if any(re.search(pattern, text_lower) for pattern in question_patterns):
        return "❓ Question"
    
    # Default category
    return "💬 General"


def parse_chat_session(data: Dict) -> List[Tuple[str, str, str, int]]:
    """
    Parse a chat session and extract conversations.
    Returns list of tuples: (type, text, formatted_time, timestamp)
    """
    conversations = []
    
    if 'requests' not in data:
        return conversations
    
    for request in data['requests']:
        timestamp = request.get('timestamp', 0)
        formatted_time = format_timestamp(timestamp)
        
        # Extract user message
        if 'message' in request:
            message = request['message']
            user_text = message.get('text', '')
            if not user_text and 'parts' in message:
                user_text = extract_text_from_parts(message['parts'])
            
            # Skip messages that begin with @agent (behind-the-scenes operations)
            if user_text and not user_text.strip().startswith('@agent'):
                conversations.append(('user', user_text, formatted_time, timestamp))
        
        # Extract response (only if we included the user message)
        if 'response' in request and conversations and conversations[-1][0] == 'user' and conversations[-1][3] == timestamp:
            response_text = extract_response_text(request['response'])
            if response_text:
                conversations.append(('assistant', response_text, formatted_time, timestamp))
    
    return conversations


def deduplicate_conversations(conversations: List[Tuple[str, str, str, int]]) -> List[Tuple[str, str, str, int]]:
    """Remove duplicate conversations with same text and timestamp."""
    seen: Set[Tuple[str, str, int]] = set()
    deduped = []
    
    for conv_type, text, formatted_time, timestamp in conversations:
        key = (conv_type, text, timestamp)
        if key not in seen:
            seen.add(key)
            deduped.append((conv_type, text, formatted_time, timestamp))
    
    return deduped


def sanitize_anchor(text: str) -> str:
    """Convert text to a valid markdown anchor."""
    # Remove special characters and replace spaces with hyphens
    anchor = text.lower()
    anchor = anchor.replace(' ', '-')
    anchor = ''.join(c for c in anchor if c.isalnum() or c == '-')
    # Limit length for readability
    return anchor[:100]


def sanitize_markdown(text: str) -> str:
    """
    Ensure markdown code blocks are properly closed.
    Counts backtick fences (both ``` and ````) and adds closing fence if needed.
    """
    # Parse lines to find fence markers
    lines = text.split('\n')
    quad_fence_lines = []
    triple_fence_lines = []
    
    for i, line in enumerate(lines):
        stripped = line.strip()
        # Check for fence markers (with or without language)
        if stripped.startswith('````'):
            quad_fence_lines.append(i)
        elif stripped.startswith('```'):
            triple_fence_lines.append(i)
    
    # Close unclosed quad fences
    if len(quad_fence_lines) % 2 != 0:
        text += '\n````\n'
    
    # Close unclosed triple fences
    elif len(triple_fence_lines) % 2 != 0:
        text += '\n```\n'
    
    return text



def get_first_line(text: str) -> str:
    """Get the first line of text."""
    lines = text.split('\n')
    return lines[0].strip()


def truncate_text(text: str, max_length: int = 80) -> str:
    """Truncate text for TOC display."""
    # Get first line only
    first_line = get_first_line(text)
    if len(first_line) <= max_length:
        return first_line
    return first_line[:max_length] + "..."


def render_markdown(all_conversations: List[Tuple[str, str, str, int]]) -> str:
    """Render all conversations chronologically as markdown with table of contents."""
    md_lines = ["# Copilot Chat History\n"]
    
    # Sort all conversations by timestamp
    sorted_convs = sorted(all_conversations, key=lambda x: x[3])
    
    # Find user prompts for TOC and categorize them
    user_prompts = []
    prompt_counter = 1
    category_counts = {}
    timestamps = []
    
    for i, (conv_type, text, formatted_time, timestamp) in enumerate(sorted_convs):
        if conv_type == 'user':
            first_line = get_first_line(text)
            category = categorize_prompt(text)
            anchor = sanitize_anchor(f"prompt-{prompt_counter}-{first_line}")
            truncated = truncate_text(text)
            user_prompts.append((prompt_counter, truncated, anchor, formatted_time, text, category))
            timestamps.append(timestamp)
            
            # Count categories
            category_counts[category] = category_counts.get(category, 0) + 1
            prompt_counter += 1
    
    # Calculate time-based statistics
    unique_days = set()
    sessions = []
    current_session_start = None
    current_session_end = None
    
    ONE_HOUR_MS = 60 * 60 * 1000  # 1 hour in milliseconds
    eastern = ZoneInfo('America/New_York')
    
    # Convert all timestamps to Eastern timezone for accurate day/session calculation
    eastern_timestamps = []
    for ts in timestamps:
        dt_utc = datetime.fromtimestamp(ts / 1000, tz=timezone.utc)
        dt_eastern = dt_utc.astimezone(eastern)
        eastern_timestamps.append((ts, dt_eastern))
        unique_days.add(dt_eastern.date())
    
    # Calculate sessions based on Eastern timezone
    for ts, dt_eastern in eastern_timestamps:
        if current_session_start is None:
            # Start new session
            current_session_start = ts
            current_session_end = ts
        else:
            # Check if this prompt is within 1 hour of the last one
            time_gap = ts - current_session_end
            if time_gap <= ONE_HOUR_MS:
                # Continue current session
                current_session_end = ts
            else:
                # Save previous session and start new one
                sessions.append((current_session_start, current_session_end))
                current_session_start = ts
                current_session_end = ts
    
    # Don't forget the last session
    if current_session_start is not None:
        sessions.append((current_session_start, current_session_end))
    
    # Calculate total hours
    total_hours = 0
    for session_start, session_end in sessions:
        session_duration_ms = session_end - session_start
        session_hours = session_duration_ms / (1000 * 60 * 60)
        # Add minimum 15 minutes per session (accounting for thinking/work time)
        total_hours += max(session_hours, 0.25)
    
    # Generate statistics section
    md_lines.append("## 📊 Chat Statistics\n")
    md_lines.append(f"**Total Prompts:** {len(user_prompts)}\n")
    md_lines.append(f"**Unique Days:** {len(unique_days)} days\n")
    md_lines.append(f"**Coding Sessions:** {len(sessions)} sessions\n")
    md_lines.append(f"**Estimated Time:** ~{total_hours:.1f} hours ({total_hours/len(unique_days):.1f} hours/day avg)\n")
    md_lines.append("\n**Prompts by Category:**\n")
    
    # Sort categories by count (descending)
    sorted_categories = sorted(category_counts.items(), key=lambda x: x[1], reverse=True)
    
    # Find max count for scaling bars
    max_count = max(category_counts.values()) if category_counts else 1
    bar_width = 30  # Maximum bar width in characters
    
    # Create table header
    md_lines.append("| Category | Count | Percentage | Distribution |")
    md_lines.append("|----------|-------|------------|--------------|")
    
    for category, count in sorted_categories:
        percentage = (count / len(user_prompts) * 100) if user_prompts else 0
        # Calculate bar length
        bar_length = int((count / max_count) * bar_width)
        bar = "█" * bar_length
        # Format the row
        md_lines.append(f"| {category} | {count} | {percentage:.1f}% | `{bar}` |")
    
    md_lines.append("\n---\n")
    
    # Generate table of contents
    md_lines.append("## Table of Contents\n")
    for num, truncated_text, anchor, formatted_time, _, category in user_prompts:
        md_lines.append(f"{num}. **{category}** [{truncated_text}](#{anchor}) - *{formatted_time}*")
    md_lines.append("\n---\n")
    
    # Generate content chronologically
    prompt_num = 1
    
    for conv_type, text, formatted_time, _ in sorted_convs:
        if conv_type == 'user':
            # Find the info for this prompt
            if prompt_num <= len(user_prompts):
                _, truncated, anchor, _, full_text, category = user_prompts[prompt_num - 1]
                first_line = get_first_line(full_text)
            
            md_lines.append(f"## <a id=\"{anchor}\"></a>{prompt_num}. {category}: {first_line}\n")
            md_lines.append("[↑ Back to Table of Contents](#table-of-contents)\n")
            md_lines.append(f"**User Prompt** *[{formatted_time}]*\n")
            # Sanitize user prompt to close any unclosed code blocks
            sanitized_text = sanitize_markdown(text)
            md_lines.append(f"{sanitized_text}\n")
            prompt_num += 1
        else:
            md_lines.append(f"### Assistant Response [{formatted_time}]\n")
            # Sanitize assistant response to close any unclosed code blocks
            sanitized_text = sanitize_markdown(text)
            md_lines.append(f"{sanitized_text}\n")
    
    return "\n".join(md_lines)


def main():
    """Main function to process all chat files."""
    # Find all JSON chat files
    chat_dir = Path('.copilot_chats')
    if not chat_dir.exists():
        print(f"Directory {chat_dir} does not exist")
        return
    
    json_files = sorted(chat_dir.glob('*.json'))
    
    if not json_files:
        print(f"No JSON files found in {chat_dir}")
        return
    
    print(f"Found {len(json_files)} chat file(s)")
    
    all_conversations = []
    all_sessions = []
    
    # Process each file
    for json_file in json_files:
        print(f"\nProcessing {json_file.name}...")
        
        try:
            data = load_chat_file(json_file)
            conversations = parse_chat_session(data)
            
            if not conversations:
                print(f"  No conversations found in {json_file.name}")
                continue
            
            print(f"  Extracted {len(conversations)} message(s)")
            
            session_name = json_file.stem
            all_sessions.append((session_name, conversations))
            all_conversations.extend(conversations)
            
        except Exception as e:
            print(f"  Error processing {json_file.name}: {e}")
    
    if not all_conversations:
        print("\nNo conversations found in any file")
        return
    
    # Deduplicate across ALL sessions globally
    print(f"\nTotal messages before deduplication: {len(all_conversations)}")
    
    # Track which messages we've seen globally (by text and timestamp)
    seen_global: Set[Tuple[str, str, int]] = set()
    deduped_all = []
    
    for session_name, conversations in all_sessions:
        for conv_type, text, formatted_time, timestamp in conversations:
            key = (conv_type, text, timestamp)
            if key not in seen_global:
                seen_global.add(key)
                deduped_all.append((conv_type, text, formatted_time, timestamp))
    
    print(f"Total messages after deduplication: {len(deduped_all)}")
    
    if len(all_conversations) > len(deduped_all):
        print(f"Removed {len(all_conversations) - len(deduped_all)} duplicate(s) across all sessions")
    
    # Generate combined markdown ordered chronologically
    markdown = render_markdown(deduped_all)
    
    # Write output
    output_file = Path('copilot_chat_history.md')
    with open(output_file, 'w', encoding='utf-8') as f:
        f.write(markdown)
    
    print(f"\nWritten combined output to {output_file}")
    print("Done!")


if __name__ == '__main__':
    main()
