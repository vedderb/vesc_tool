# Markdown Definitions

This specification defines which markdown syntax can be parsed by maddy.
There is no HTML allowed in the markdown syntax - or said otherwise - it might
destroy the output, if there was HTML in your markdown.

The Parser expects you to use spaces and not tabs for indentation in the
markdown.

If a line starts with `<` and `config->isHTMLWrappedInParagraph` is false, it
expects that the upcoming line is HTML and therefor will not be surrounded by a
paragraph.

## Headlines

```
# h1 heading
## h2 heading
### h3 heading
#### h4 heading
##### h5 heading
###### h6 heading
```
results in:
```html
<h1>h1 heading</h1>
<h2>h2 heading</h2>
<h3>h3 heading</h3>
<h4>h4 heading</h4>
<h5>h5 heading</h5>
<h6>h6 heading</h6>
```

## Links

```
[Text of the link](http://example.com)
```
results in
```html
<a href="http://example.com">Text of the link</a>
```

## Lists

### unordered
Characters "*", "+" or "-" to make an unordered "bullet" list are equivalent.

```

- unordered
* list
+ items

```
results in
```html
<ul>
  <li>unordered</li>
  <li>list</li>
  <li>items</li>
</ul>
```

```

* unordered
  * list
  * items
    * in
    + an
  - hierarchy

```
results in
```html
<ul>
  <li>unordered
    <ul>
      <li>list</li>
      <li>items
        <ul>
          <li>in</li>
          <li>an</li>
        </ul>
      </li>
      <li>hierarchy</li>
    </ul>
  </li>
</ul>
```

### ordered

```

1. ordered
2. list
3. items

```

results in

```html

<ol>
  <li>ordered</li>
  <li>list</li>
  <li>items</li>
</ol>

```

```

1. ordered
* list
* items

```

results in

```html

<ol>
  <li>ordered</li>
  <li>list</li>
  <li>items</li>
</ol>

```

```

1. ordered
* list
  1. items
  * in
    1. an
  * hierarchy

```

results in

```html
<ol>
  <li>ordered</li>
  <li>list
    <ol>
      <li>items</li>
      <li>in
        <ol>
          <li>an</li>
        </ol>
      </li>
      <li>hierarchy</li>
    </ol>
  </li>
</ol>
```

### combination

```

* combination
* of
  1. unordered and
  * ordered
* list

```
results in
```html
<ul>
  <li>combination</li>
  <li>of
    <ol>
      <li>unordered and</li>
      <li>ordered</li>
    </ol>
  </li>
  <li>list</li>
</ul>
```

### checklist

```

- [ ] some item
  - [ ] another item
- [x] some checked item

```
results in
```html
<ul class="checklist">
  <li><label><input type="checkbox"/>some item
    <ul class="checklist">
      <li><label><input type="checkbox"/><span>another item</label></li>
    </ul>
  </label></li>
  <li><label><input type="checkbox" checked="checked"/>some checked item</label></li>
</ul>
```
might not work in combination with other lists

## Code Blocks

    ```
    some code
    ```

results in
```html
<pre><code>
some code
</code></pre>
```

    ```cpp
    int a = 42;
    ```

results in

```html
<pre class="cpp"><code>
int a = 42;
</code></pre>
```

## Inline code

    some text `some inline code` some other text

results in
```html
some text <code>some inline code</code> some other text
```

## quotes

```
> Some quote
```
results in
```html
<blockqoute>
  <p>Some quote</p>
</blockquote>
```

## bold

```
**bold text**
__bold text__
```
results in
```html
<strong>bold text</strong>
<strong>bold text</strong>
```

## italic

```
*italic text*
```
results in
```html
<i>italic text</i>
```

## emphasized

This can be disabled by setting `config->isEmphasizedParserEnabled = false`.

```
_emphasized text_
```
results in
```html
<em>emphasized text</em>
```

## strikethrough

```
~~striked through text~~
```
results in
```html
<s>striked through text</s>
```

## horizontal line

```
---
```
results in
```html
<hr/>
```

## break line

```
New\r\nLine
```
results in
```html
New<br>
Line
```

## Images

```
![Image alt text](http://example.com/example.png)
```
results in
```html
<img src="http://example.com/example.png" alt="Image alt text"/>
```

## Tables

```

|table>
Left header | middle header | last header
- | - | -
cell 1 | cell 2 | cell 3
cell 4 | cell 5 | cell 6
- | - | -
foot a | foot b | foot c
|<table

```
becomes
```html
<table>
  <thead>
    <tr>
      <th>Left header</th>
      <th>middle header</th>
      <th>last header</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td>cell 1</td>
      <td>cell 2</td>
      <td>cell 3</td>
    </tr>
    <tr>
      <td>cell 4</td>
      <td>cell 5</td>
      <td>cell 6</td>
    </tr>
  </tbody>
  <tfoot>
    <tr>
      <td>foot a</td>
      <td>foot b</td>
      <td>foot c</td>
    </tr>
  </tfoot>
</table>
```
table header and footer are optional

## LaTeX(MathJax) block support

To turn on the LaTeX support - which basically is only a
[MathJax](https://www.mathjax.org/) support and makes sure, that formulas aren't
internally checked for other parsers - it has to be enabled in config:

```cpp
std::shared_ptr<maddy::ParserConfig> config = std::make_shared<maddy::ParserConfig>();
config->enabledParsers |= maddy::types::LATEX_BLOCK_PARSER;

std::shared_ptr<maddy::Parser> parser = std::make_shared<maddy::Parser>(config);
std::string htmlOutput = parser->Parse(markdownInput);
```

After this you can do the following in Markdown:

```
$$x = {-b \\pm \\sqrt{b^2-4ac} \\over 2a}.$$
```

Which results in

```html
$$x = {-b \\pm \\sqrt{b^2-4ac} \\over 2a}.$$\n
```
