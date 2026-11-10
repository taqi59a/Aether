#!/usr/bin/env python3
"""Aether EdTech site generator. Run:  python3 tools/build.py
Outputs static pages into site/{en,fr,nl,de}/ and site/index.html (language router)."""
import os, re, shutil

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "site")
LANGS = ["en", "fr", "nl", "de"]
LANG_NAME = {"en": "English", "fr": "Français", "nl": "Nederlands", "de": "Deutsch"}
PAGES = ["index", "about", "projects", "services", "support", "contact", "legal", "privacy", "cookies"]
SUPPORTERS = ["Google", "Microsoft", "GoodStack", "Adobe", "Canva", "StartLab Brussels"]
EMAIL = "info@myaether.org"

# =====================================================================
# STRINGS  (HTML allowed; use single quotes for attributes)
# =====================================================================
S = {}

S["en"] = dict(
 skip="Skip to content", menu="Menu", langlabel="Language",
 n_index="Home", n_about="About", n_projects="Projects", n_services="Services", n_contact="Contact", n_support="Support us",
 f_tag="A non-profit working for education and educational projects, from Schaerbeek, Brussels.",
 f_explore="Explore", f_project="Project", f_partner="Partner with us", f_reach="Reach us", f_legal="Legal",
 f_status="<strong>Legal status:</strong> Aether EdTech is an <em>association sans but lucratif</em> (ASBL / VZW) currently being registered in Brussels. Enterprise number (BCE/KBO) pending. Until registration is complete we do not issue tax certificates. <a href='legal.html'>Details</a>",
 f_eu="Hosted in the EU · No cookies · No tracking",
 t_index="Aether EdTech · Education non-profit in Brussels",
 d_index="Aether EdTech is a non-profit (ASBL) in Schaerbeek, Brussels, supporting education and educational projects. Creators of BacMath.eu.",
 t_about="About · Aether EdTech", d_about="Meet the founders of Aether EdTech, a Brussels education non-profit: Taqi Abbas and Minha Shahid. Our cause, values and legal status.",
 t_projects="Projects · Aether EdTech", d_projects="BacMath.eu, Aether EdTech's free learning website, and the other educational initiatives we take part in.",
 t_services="Services · Aether EdTech", d_services="Software development and digital advice from a Brussels non-profit. Surplus funds free educational projects.",
 t_support="Support us · Aether EdTech", d_support="Donate, volunteer or partner with Aether EdTech to keep free educational projects running.",
 t_contact="Contact · Aether EdTech", d_contact="Contact Aether EdTech in Schaerbeek, Brussels, about donations, partnerships, volunteering or software projects.",
 t_legal="Legal notice · Aether EdTech", d_legal="Legal status, registration progress and publisher details of Aether EdTech, Brussels.",
 t_privacy="Privacy policy & GDPR · Aether EdTech", d_privacy="How Aether EdTech protects your data: no tracking, EU hosting, your GDPR rights and how to use them.",
 t_cookies="Cookie statement · Aether EdTech", d_cookies="This website uses no cookies and no tracking technologies.",
 # home
 h_eyebrow="Non-profit · Education · Brussels",
 h_h1="Good learning should <span class='scribble'>reach everyone</span>, wherever they start.",
 h_lede="Aether EdTech is a Belgian non-profit (ASBL) that supports education and educational projects. We build free learning tools, take part in initiatives that share our cause, and fund the work through honest, mission-driven software services.",
 h_b1="Try BacMath.eu", h_b2="Support our work",
 h_pill="Aether EdTech · Schaerbeek, Brussels · registration in progress",
 id_h="At a glance", id1="Based in", id1v="Schaerbeek, Brussels, Belgium", id2="Status", id2v="ASBL, registration in progress",
 id3="Running project", id3v="BacMath.eu", id4="Founders", id4v="Taqi Abbas &amp; Minha Shahid",
 w_eyebrow="What we do", w_h2="Three ways we work for education.", w_lede="We are teachers and engineers. That mix shapes everything we make.",
 r1h="We build learning tools", r1p="Websites and software designed with the classroom in mind: clear, playful, and free for the learner. BacMath.eu is the first, and it is live today.",
 r2h="We join other initiatives", r2p="Education is rarely fixed by one organisation alone. We contribute skills, software and time to projects run by schools, associations and community groups.",
 r3h="We fund it through service", r3p="We build software for schools, organisations and companies. Any surplus goes back into free educational projects, because an ASBL cannot distribute profit.",
 fe_eyebrow="Our running project",
 fe_p1="A maths learning website that treats practice as play instead of punishment. Students learn at their own pace, in a format that makes them want to come back tomorrow.",
 fe_p2="It is free to use, and it stays free because people like you help us keep it running.",
 fe_t1="Maths explained in a fun, approachable way", fe_t2="Open to every learner, no fee to start", fe_t3="Built and maintained by teachers and engineers",
 fe_b1="Visit BacMath.eu", fe_b2="About the project",
 fe_q="Learn it.<br>Play with it.<br>Keep it.", fe_cap="BacMath.eu",
 th_eyebrow="With thanks", th_h2="We are not doing this alone.", th_lede="Aether EdTech is young, and these organisations have already backed it. We are grateful.",
 th_fine="Names shown as text to acknowledge support. They are trademarks of their respective owners, and this acknowledgement does not imply endorsement of Aether EdTech.",
 b_h2="Help us keep learning free.", b_p="Donations pay for hosting, development time and new lessons. If you cannot give money, you can give skills, time or an introduction. All of it counts.", b_btn="See how to help",
 # about
 ab_eyebrow="About Aether EdTech", ab_h1="Two teachers, one codebase, one cause.",
 ab_lede="Aether EdTech began with a plain observation: a lot of good teaching never travels beyond the room it happens in. We want to change that, with software that carries it further.",
 c_eyebrow="Our cause", c_h2="Education that does not depend on where you were born.",
 c_p1="Aether EdTech exists to support education and educational projects. That is a broad sentence, so here is what it means in practice.",
 c_p2="We create learning resources that are free for the learner. We build and maintain the software behind them. And we take part in other initiatives, offering technical and teaching skills to people who are already doing good work and could use another pair of hands.",
 bel_h="What we believe",
 bel1="<strong>Learning should be enjoyable.</strong> A student who is curious learns more than one who is afraid.",
 bel2="<strong>Access should not be a privilege.</strong> Core educational tools stay free to use.",
 bel3="<strong>Be honest about what we are.</strong> We are small, we are new, and we say so.",
 bel4="<strong>Reinvest, never extract.</strong> Any surplus goes back into educational work.",
 fo_eyebrow="Founders", fo_h2="The people behind it.",
 p1_role="Founder", p1_prof="Educationist · Teacher · Software Engineer",
 p1_bio="Taqi builds the tools and keeps the projects running, with a classroom perspective on what is actually useful.",
 p2_role="Co-Founder", p2_prof="Educationist · Teacher · Doctor",
 p2_bio="Minha brings a teacher's eye for how people learn, and the care that comes with working in medicine.",
 wh_h="Where we are", wh_p="Schaerbeek, Brussels, Belgium. This website is available in English, French, Dutch and German.",
 st_h="Our legal status",
 st_p="<strong>Registration in progress.</strong> Aether EdTech is being constituted as an <em>association sans but lucratif</em> (ASBL) in Brussels. We will publish the enterprise number and the statutes as soon as the registration is complete.",
 # projects
 pr_eyebrow="Projects", pr_h1="What we are building, and what we are joining.",
 pr_lede="Free educational resources are our public good. They are open to everyone and they come first.",
 pr_live="Live now",
 pr_p1="BacMath.eu is an educational maths website that makes learning fun. It is our main running project and the clearest picture of what Aether EdTech is for: take a subject many students dread, and make it something they can enjoy.",
 pr_p2="It is free to use. Hosting, development and new content cost time and money, which is why your support matters.",
 pr_b1="Open BacMath.eu", pr_b2="Help fund it", pr_q="Learn it.<br>Play with it.<br>Keep it.", pr_cap="The idea behind BacMath",
 ot_eyebrow="Beyond BacMath", ot_h2="Other initiatives.",
 ot_p1="We take part in initiatives that share our cause. Sometimes that means building a tool for a school or an association. Sometimes it means giving teaching time, advice or an extra developer for a few weeks.",
 ot_p2="We will list partner projects here as they are confirmed. We would rather show a short, true list than a long, padded one.",
 ot_h3="Have a project in mind?", ot_p3="If you run an educational project and need software skills, or you teach and have an idea that needs building, tell us about it.", ot_btn="Start a conversation",
 # services
 sv_eyebrow="Services", sv_h1="Software for hire, for a purpose.",
 sv_lede="We build websites and software for schools, associations and companies. What remains after costs funds free educational projects like BacMath.eu.",
 sv_how="How it works", sv_h2="Two sides of one organisation.",
 sv_f_h="Free, always", sv_f_p="Our public educational resources, starting with BacMath.eu. Open to every learner, funded by donations, partners and the services on this page.",
 sv_p_h="Paid, for a reason", sv_p_p="Software projects for organisations that need them. Fees cover our costs, and any surplus is reinvested in education, as the law for an ASBL requires.",
 sv1h="Websites and web applications", sv1p="Clear, fast, accessible sites and tools for schools, associations and small organisations.",
 sv2h="Educational software", sv2p="Learning platforms, quizzes, practice tools and content systems, built with a teacher's input from the first sketch.",
 sv3h="Digital advice", sv3p="Practical guidance for institutions on choosing tools, going online and making their digital services easier to use.",
 sv_b_h="Commission something that does good twice.", sv_b_p="You get the software you need. Education gets the surplus. Tell us what you are planning.", sv_b_btn="Get in touch",
 # support
 su_eyebrow="Support us", su_h1="Keep learning within reach.",
 su_lede="We are a small team with a clear job. Money, skills and introductions all help us do it for longer.",
 w1h="Donate", w1p="Your gift pays for hosting, development time and new lessons. We are finalising our bank account as part of registration. To give now, write to us and we will send transfer details.", w1a="Ask for donation details",
 w2h="Give skills or time", w2p="Teachers, developers, designers, translators: if you can give a few hours, there is probably a task for you.", w2a="Offer your skills",
 w3h="Partner with us", w3p="Companies and foundations can sponsor a project, supply tools, or commission our services knowing the surplus goes to education.", w3a="Talk partnership",
 w4h="Spread the word", w4p="Share BacMath.eu with a student, a parent or a teacher. It costs nothing and it is how most people find us.", w4a="Share BacMath.eu",
 don_subj="Donation to Aether EdTech",
 sg_eyebrow="Straight talk", sg_h2="What you should know before giving.", sg_p="We would rather you hear this from us than find out later.",
 sg1="<strong>We are not registered yet.</strong> Aether EdTech is in the process of registration in Brussels. We will announce it on this site.",
 sg2="<strong>No tax certificate for now.</strong> In Belgium, tax-deductible receipts require formal recognition. We do not have it and will not claim it. If that changes, we will say so.",
 sg3="<strong>Where the money goes.</strong> Hosting, software tools, development and educational content for our projects. We will publish a simple annual summary once we are registered.",
 so_eyebrow="Our supporters", so_h2="Thank you.",
 so_note="We are especially grateful to <strong>StartLab Brussels</strong>, whose support helps us get started in Brussels.",
 # contact
 ct_eyebrow="Contact", ct_h1="Write to us.", ct_lede="Questions, ideas, offers of help, requests for a quote: they all arrive in the same inbox, and a person answers.",
 ct_email="Email", ct_place="Place", ct_project="Project", ct_status="Status", ct_status_v="ASBL registration in progress",
 ct_name="Your name", ct_mail="Your email", ct_topic="I am writing about", ct_msg="Message",
 ct_o1="A donation", ct_o2="Offering skills or time", ct_o3="A partnership", ct_o4="A software project", ct_o5="BacMath.eu", ct_o6="Something else",
 ct_btn="Open in my email app", ct_fine="This opens your own email program with the message ready to send. Nothing is sent to or stored on this website. See our <a href='privacy.html'>privacy policy</a>.",
 # common labels
 upd="Last updated: 7 November 2026",
 # legal
 lg_eyebrow="Legal notice", lg_h1="Legal notice.", lg_lede="For the public, for partners and for the authorities.",
 lg_who="Publisher",
 lg_k=["Name","Legal form","Status","Enterprise number","Location","Founder","Co-founder","Contact","Website","Hosting"],
 lg_v=["Aether EdTech","<em>Association sans but lucratif</em> (ASBL / VZW), under Belgian law","<strong>In the process of registration</strong> in Brussels","Pending (BCE / KBO)","Schaerbeek, Brussels, Belgium","Taqi Abbas","Minha Shahid","<a href='mailto:"+EMAIL+"'>"+EMAIL+"</a>","myaether.org","Servers located in the European Union"],
 lg_after="This page will be updated with the full registered address, enterprise number and a link to the statutes once registration is complete and published in the Belgian Official Gazette (<em>Moniteur belge</em>).",
 lg_sections=[
  ("Donations and receipts","Until the registration is complete, Aether EdTech cannot issue tax certificates for donations, and no donor should expect a tax deduction. Any change will be announced on this site."),
  ("Content and intellectual property","Texts, design and code of this website belong to Aether EdTech unless stated otherwise. You may quote short extracts with a link to the source. BacMath.eu and other linked projects have their own terms."),
  ("Liability","We take care to keep this site accurate, but we cannot guarantee that it is free of errors. We are not responsible for the content of external sites that we link to."),
  ("Trademarks","Google, Microsoft, GoodStack, Adobe, Canva and StartLab Brussels are named here only to thank them for their support. Their names are trademarks of their respective owners and do not imply endorsement of Aether EdTech."),
  ("Data protection","See our <a href='privacy.html'>privacy policy and GDPR information</a> and our <a href='cookies.html'>cookie statement</a>."),
  ("Applicable law","Belgian law applies. Courts of Brussels have jurisdiction, without prejudice to the mandatory rights of consumers."),
 ],
 # privacy
 pv_eyebrow="Privacy &amp; GDPR", pv_h1="Privacy policy and GDPR information.",
 pv_lede="Short version: we do not track you, we do not use cookies, and your data stays in the European Union.",
 pv_sections=[
  ("In short","<ul><li>No cookies, no analytics, no advertising, no tracking pixels.</li><li>No data about your visit is collected by us, and nothing is sold or shared for marketing.</li><li>This website is hosted on servers located in the European Union.</li><li>We do not transfer personal data outside the European Economic Area.</li><li>You can email us at any time to exercise your rights.</li></ul>"),
  ("Who is responsible","The controller of personal data is <strong>Aether EdTech</strong>, Schaerbeek, Brussels, Belgium (registration in progress). Contact: <a href='mailto:"+EMAIL+"'>"+EMAIL+"</a>. We are not required to appoint a Data Protection Officer; privacy questions go to the same address."),
  ("What we process, why, and for how long","<h3>Visiting this website</h3><p>We do not collect personal data from visitors. Like every web server, our hosting provider may technically record your IP address and the time of your request in short-lived security logs, held in the EU, so the site can run safely. We do not use these logs to identify or profile visitors.</p><h3>Language selection</h3><p>The language of the site is chosen by a small script that reads your browser's language setting on your own device. This information is not sent to us and nothing is stored.</p><h3>When you email us</h3><p>We process your name, email address and the content of your message to reply to you. Legal basis: our legitimate interest in answering enquiries, or steps you ask us to take before an agreement (Art. 6(1)(f) and (b) GDPR). We keep messages only as long as needed to handle your request, normally no longer than 24 months after the last exchange.</p><h3>Donations by bank transfer</h3><p>Your bank provides us with your name, account number and the amount. We keep this to meet our accounting obligations under Belgian law (Art. 6(1)(c) GDPR), normally for seven years.</p>"),
  ("Where your data is kept","Our website and mailbox are operated within the European Union. We choose providers that process data in the EU/EEA and that act only on our instructions. We do not transfer personal data to countries outside the European Economic Area. If this ever changes, we will tell you first and rely only on the safeguards the GDPR requires."),
  ("Your rights under the GDPR","<p>You have the right to:</p><ul><li>access the personal data we hold about you;</li><li>have inaccurate data corrected;</li><li>have your data erased (&ldquo;right to be forgotten&rdquo;);</li><li>restrict or object to processing;</li><li>receive your data in a portable format;</li><li>withdraw consent at any time, where processing is based on consent.</li></ul><p>Write to <a href='mailto:"+EMAIL+"'>"+EMAIL+"</a>. We reply within one month. We may ask you to confirm your identity.</p>"),
  ("Right to complain","If you think we handle your data unlawfully, you can complain to the Belgian Data Protection Authority: Rue de la Presse 35, 1000 Brussels, <a href='mailto:contact@apd-gba.be'>contact@apd-gba.be</a>, <a href='https://www.autoriteprotectiondonnees.be' rel='noopener'>autoriteprotectiondonnees.be</a>. We would appreciate the chance to put things right first."),
  ("Children","We do not knowingly collect personal data from children through this website. BacMath.eu is a separate website with its own privacy information."),
  ("Links to other websites","Links to BacMath.eu and other sites lead to services we do not control for this website. Their own privacy policies apply once you leave."),
  ("Security","We limit access to the mailbox to the founders and use reputable EU-based providers. No method is perfectly secure, and we will notify you and the authority if a breach affects you, as the GDPR requires."),
  ("Changes","We will update this page if our practices change and show the date at the top. Material changes will be announced on the home page."),
 ],
 # cookies
 ck_eyebrow="Cookies", ck_h1="Cookie statement.", ck_lede="This website uses no cookies.",
 ck_sections=[
  ("What we use","None. We set no cookies, and we do not use local storage, fingerprinting, analytics, advertising or social media widgets."),
  ("Fonts and resources","Fonts, images and scripts are served from this website itself. We do not load them from third parties, so your browser does not contact other companies when you visit."),
  ("If this changes","If we ever add anything that needs your consent, we will ask before it loads, as Belgian and EU law require, and we will update this page."),
  ("More information","See the <a href='privacy.html'>privacy policy and GDPR information</a>."),
 ],
)

S["fr"] = dict(
 skip="Aller au contenu", menu="Menu", langlabel="Langue",
 n_index="Accueil", n_about="À propos", n_projects="Projets", n_services="Services", n_contact="Contact", n_support="Nous soutenir",
 f_tag="Une association sans but lucratif au service de l'éducation et des projets éducatifs, à Schaerbeek, Bruxelles.",
 f_explore="Explorer", f_project="Projet", f_partner="Devenir partenaire", f_reach="Nous joindre", f_legal="Informations légales",
 f_status="<strong>Statut juridique :</strong> Aether EdTech est une association sans but lucratif (ASBL) en cours d'enregistrement à Bruxelles. Numéro d'entreprise (BCE) en attente. Tant que l'enregistrement n'est pas finalisé, nous n'émettons pas d'attestations fiscales. <a href='legal.html'>Détails</a>",
 f_eu="Hébergé dans l'UE · Aucun cookie · Aucun suivi",
 t_index="Aether EdTech · Association pour l'éducation à Bruxelles",
 d_index="Aether EdTech est une association sans but lucratif (ASBL) à Schaerbeek, Bruxelles, qui soutient l'éducation et les projets éducatifs. Créateurs de BacMath.eu.",
 t_about="À propos · Aether EdTech", d_about="Les fondateurs d'Aether EdTech, association bruxelloise pour l'éducation : Taqi Abbas et Minha Shahid. Notre cause, nos valeurs, notre statut.",
 t_projects="Projets · Aether EdTech", d_projects="BacMath.eu, le site d'apprentissage gratuit d'Aether EdTech, et les autres initiatives éducatives auxquelles nous participons.",
 t_services="Services · Aether EdTech", d_services="Développement logiciel et conseil numérique par une association bruxelloise. Le surplus finance des projets éducatifs gratuits.",
 t_support="Nous soutenir · Aether EdTech", d_support="Faites un don, offrez votre temps ou devenez partenaire d'Aether EdTech pour maintenir des projets éducatifs gratuits.",
 t_contact="Contact · Aether EdTech", d_contact="Contactez Aether EdTech à Schaerbeek, Bruxelles, pour un don, un partenariat, du bénévolat ou un projet logiciel.",
 t_legal="Mentions légales · Aether EdTech", d_legal="Statut juridique, avancement de l'enregistrement et éditeur d'Aether EdTech, Bruxelles.",
 t_privacy="Politique de confidentialité et RGPD · Aether EdTech", d_privacy="Comment Aether EdTech protège vos données : aucun suivi, hébergement dans l'UE, vos droits RGPD et comment les exercer.",
 t_cookies="Déclaration sur les cookies · Aether EdTech", d_cookies="Ce site n'utilise aucun cookie ni technologie de suivi.",
 h_eyebrow="Association sans but lucratif · Éducation · Bruxelles",
 h_h1="Bien apprendre doit <span class='scribble'>être accessible à tous</span>, quel que soit le point de départ.",
 h_lede="Aether EdTech est une association sans but lucratif (ASBL) belge qui soutient l'éducation et les projets éducatifs. Nous créons des outils d'apprentissage gratuits, participons à des initiatives qui partagent notre cause et finançons ce travail par des services logiciels honnêtes, au service de notre mission.",
 h_b1="Essayer BacMath.eu", h_b2="Soutenir notre travail",
 h_pill="Aether EdTech · Schaerbeek, Bruxelles · enregistrement en cours",
 id_h="En un coup d'œil", id1="Siège", id1v="Schaerbeek, Bruxelles, Belgique", id2="Statut", id2v="ASBL, enregistrement en cours",
 id3="Projet en cours", id3v="BacMath.eu", id4="Fondateurs", id4v="Taqi Abbas &amp; Minha Shahid",
 w_eyebrow="Ce que nous faisons", w_h2="Trois façons d'agir pour l'éducation.", w_lede="Nous sommes enseignants et ingénieurs. Ce mélange façonne tout ce que nous créons.",
 r1h="Nous créons des outils d'apprentissage", r1p="Des sites et des logiciels pensés pour la classe : clairs, ludiques et gratuits pour l'apprenant. BacMath.eu est le premier, et il est en ligne dès aujourd'hui.",
 r2h="Nous rejoignons d'autres initiatives", r2p="L'éducation se construit rarement seul. Nous apportons compétences, logiciels et temps à des projets portés par des écoles, des associations et des groupes citoyens.",
 r3h="Nous finançons le tout par nos services", r3p="Nous développons des logiciels pour des écoles, des organisations et des entreprises. Tout excédent retourne vers des projets éducatifs gratuits, car une ASBL ne peut pas distribuer de bénéfices.",
 fe_eyebrow="Notre projet en cours",
 fe_p1="Un site d'apprentissage des mathématiques qui fait de l'entraînement un jeu plutôt qu'une punition. Chacun avance à son rythme, dans un format qui donne envie de revenir demain.",
 fe_p2="Il est gratuit, et il le reste grâce à celles et ceux qui nous aident à le faire vivre.",
 fe_t1="Les maths expliquées de façon simple et amusante", fe_t2="Ouvert à tous les apprenants, sans frais pour commencer", fe_t3="Conçu et entretenu par des enseignants et des ingénieurs",
 fe_b1="Visiter BacMath.eu", fe_b2="À propos du projet",
 fe_q="Apprendre.<br>Jouer.<br>Retenir.", fe_cap="BacMath.eu",
 th_eyebrow="Avec nos remerciements", th_h2="Nous ne sommes pas seuls.", th_lede="Aether EdTech est jeune, et ces organisations l'ont déjà soutenu. Merci à elles.",
 th_fine="Noms affichés en texte pour reconnaître leur soutien. Ce sont des marques déposées de leurs propriétaires respectifs ; cette mention n'implique aucune approbation d'Aether EdTech.",
 b_h2="Aidez-nous à garder l'apprentissage gratuit.", b_p="Les dons financent l'hébergement, le temps de développement et de nouvelles leçons. Si vous ne pouvez pas donner d'argent, vous pouvez donner des compétences, du temps ou une introduction. Tout compte.", b_btn="Voir comment aider",
 ab_eyebrow="À propos d'Aether EdTech", ab_h1="Deux enseignants, un code, une cause.",
 ab_lede="Aether EdTech est née d'un constat simple : beaucoup de bon enseignement ne dépasse jamais la salle où il a lieu. Nous voulons changer cela, avec des logiciels qui le portent plus loin.",
 c_eyebrow="Notre cause", c_h2="Une éducation qui ne dépend pas du lieu de naissance.",
 c_p1="Aether EdTech existe pour soutenir l'éducation et les projets éducatifs. La formule est large ; voici ce qu'elle signifie concrètement.",
 c_p2="Nous créons des ressources d'apprentissage gratuites pour l'apprenant. Nous développons et maintenons les logiciels qui les font fonctionner. Et nous participons à d'autres initiatives, en offrant nos compétences techniques et pédagogiques à celles et ceux qui font déjà du bon travail et qui ont besoin d'un coup de main.",
 bel_h="Ce en quoi nous croyons",
 bel1="<strong>Apprendre doit être un plaisir.</strong> Un élève curieux apprend plus qu'un élève qui a peur.",
 bel2="<strong>L'accès ne doit pas être un privilège.</strong> Les outils éducatifs essentiels restent gratuits.",
 bel3="<strong>Être honnête sur ce que nous sommes.</strong> Nous sommes petits, nous sommes nouveaux, et nous le disons.",
 bel4="<strong>Réinvestir, jamais prélever.</strong> Tout excédent retourne au travail éducatif.",
 fo_eyebrow="Fondateurs", fo_h2="Les personnes derrière Aether EdTech.",
 p1_role="Fondateur", p1_prof="Éducateur · Enseignant · Ingénieur logiciel",
 p1_bio="Taqi crée les outils et fait vivre les projets, avec un regard de classe sur ce qui est réellement utile.",
 p2_role="Cofondatrice", p2_prof="Éducatrice · Enseignante · Médecin",
 p2_bio="Minha apporte le regard d'une enseignante sur la façon dont on apprend, et l'attention propre au métier de médecin.",
 wh_h="Où nous sommes", wh_p="Schaerbeek, Bruxelles, Belgique. Ce site est disponible en anglais, français, néerlandais et allemand.",
 st_h="Notre statut juridique",
 st_p="<strong>Enregistrement en cours.</strong> Aether EdTech est en cours de constitution en <em>association sans but lucratif</em> (ASBL) à Bruxelles. Nous publierons le numéro d'entreprise et les statuts dès que l'enregistrement sera finalisé.",
 pr_eyebrow="Projets", pr_h1="Ce que nous construisons, et ce que nous rejoignons.",
 pr_lede="Les ressources éducatives gratuites sont notre bien commun. Elles sont ouvertes à tous et passent en premier.",
 pr_live="En ligne",
 pr_p1="BacMath.eu est un site éducatif de mathématiques qui rend l'apprentissage amusant. C'est notre principal projet en cours et la meilleure image de la raison d'être d'Aether EdTech : prendre une matière que beaucoup redoutent, et en faire quelque chose qu'on peut aimer.",
 pr_p2="Il est gratuit. L'hébergement, le développement et les nouveaux contenus demandent du temps et de l'argent : votre soutien compte.",
 pr_b1="Ouvrir BacMath.eu", pr_b2="Aider à le financer", pr_q="Apprendre.<br>Jouer.<br>Retenir.", pr_cap="L'idée derrière BacMath",
 ot_eyebrow="Au-delà de BacMath", ot_h2="Autres initiatives.",
 ot_p1="Nous participons à des initiatives qui partagent notre cause. Parfois, il s'agit de créer un outil pour une école ou une association. Parfois, de donner du temps d'enseignement, des conseils ou un développeur supplémentaire pour quelques semaines.",
 ot_p2="Nous listerons ici les projets partenaires au fur et à mesure de leur confirmation. Nous préférons une liste courte et vraie à une liste longue et gonflée.",
 ot_h3="Un projet en tête ?", ot_p3="Vous portez un projet éducatif et avez besoin de compétences logicielles, ou vous enseignez et avez une idée à réaliser ? Parlez-nous-en.", ot_btn="Engager la conversation",
 sv_eyebrow="Services", sv_h1="Du logiciel sur mesure, pour une raison.",
 sv_lede="Nous développons des sites et des logiciels pour des écoles, des associations et des entreprises. Ce qui reste après les coûts finance des projets éducatifs gratuits comme BacMath.eu.",
 sv_how="Comment ça marche", sv_h2="Deux facettes d'une même organisation.",
 sv_f_h="Gratuit, toujours", sv_f_p="Nos ressources éducatives publiques, à commencer par BacMath.eu. Ouvertes à tous, financées par les dons, les partenaires et les services de cette page.",
 sv_p_h="Payant, pour une raison", sv_p_p="Des projets logiciels pour les organisations qui en ont besoin. Les honoraires couvrent nos coûts, et tout excédent est réinvesti dans l'éducation, comme l'exige la loi pour une ASBL.",
 sv1h="Sites et applications web", sv1p="Des sites et outils clairs, rapides et accessibles pour les écoles, associations et petites organisations.",
 sv2h="Logiciels éducatifs", sv2p="Plateformes d'apprentissage, quiz, outils d'entraînement et systèmes de contenu, conçus avec l'avis d'un enseignant dès le premier croquis.",
 sv3h="Conseil numérique", sv3p="Des conseils pratiques pour choisir des outils, passer en ligne et rendre les services numériques plus simples à utiliser.",
 sv_b_h="Commandez quelque chose qui fait deux fois du bien.", sv_b_p="Vous obtenez le logiciel dont vous avez besoin. L'éducation reçoit l'excédent. Parlez-nous de votre projet.", sv_b_btn="Nous contacter",
 su_eyebrow="Nous soutenir", su_h1="Garder l'apprentissage à portée de tous.",
 su_lede="Nous sommes une petite équipe avec une mission claire. L'argent, les compétences et les mises en relation nous aident à tenir plus longtemps.",
 w1h="Faire un don", w1p="Votre don finance l'hébergement, le temps de développement et de nouvelles leçons. Notre compte bancaire est en cours de finalisation avec l'enregistrement. Pour donner dès maintenant, écrivez-nous et nous vous enverrons les coordonnées de virement.", w1a="Demander les coordonnées de don",
 w2h="Donner compétences ou temps", w2p="Enseignants, développeurs, designers, traducteurs : si vous pouvez donner quelques heures, il y a sans doute une tâche pour vous.", w2a="Proposer vos compétences",
 w3h="Devenir partenaire", w3p="Entreprises et fondations peuvent parrainer un projet, fournir des outils ou commander nos services en sachant que l'excédent va à l'éducation.", w3a="Parler partenariat",
 w4h="Passer le mot", w4p="Partagez BacMath.eu avec un élève, un parent ou un enseignant. Cela ne coûte rien et c'est ainsi que la plupart des gens nous trouvent.", w4a="Partager BacMath.eu",
 don_subj="Don à Aether EdTech",
 sg_eyebrow="En toute franchise", sg_h2="Ce qu'il faut savoir avant de donner.", sg_p="Mieux vaut que vous l'appreniez de nous plutôt que plus tard.",
 sg1="<strong>Nous ne sommes pas encore enregistrés.</strong> Aether EdTech est en cours d'enregistrement à Bruxelles. Nous l'annoncerons sur ce site.",
 sg2="<strong>Pas d'attestation fiscale pour l'instant.</strong> En Belgique, les attestations déductibles exigent une reconnaissance formelle. Nous ne l'avons pas et ne prétendrons pas l'avoir. Si cela change, nous le dirons.",
 sg3="<strong>Où va l'argent.</strong> Hébergement, outils logiciels, développement et contenu éducatif pour nos projets. Nous publierons un bilan annuel simple une fois enregistrés.",
 so_eyebrow="Nos soutiens", so_h2="Merci.",
 so_note="Nous remercions tout particulièrement <strong>StartLab Brussels</strong>, dont le soutien nous aide à démarrer à Bruxelles.",
 ct_eyebrow="Contact", ct_h1="Écrivez-nous.", ct_lede="Questions, idées, propositions d'aide, demandes de devis : tout arrive dans la même boîte, et une vraie personne répond.",
 ct_email="E-mail", ct_place="Lieu", ct_project="Projet", ct_status="Statut", ct_status_v="Enregistrement ASBL en cours",
 ct_name="Votre nom", ct_mail="Votre e-mail", ct_topic="Je vous écris au sujet de", ct_msg="Message",
 ct_o1="Un don", ct_o2="Une offre de compétences ou de temps", ct_o3="Un partenariat", ct_o4="Un projet logiciel", ct_o5="BacMath.eu", ct_o6="Autre chose",
 ct_btn="Ouvrir dans ma messagerie", ct_fine="Cela ouvre votre propre messagerie avec le message prêt à envoyer. Rien n'est envoyé ni conservé sur ce site. Voir notre <a href='privacy.html'>politique de confidentialité</a>.",
 upd="Dernière mise à jour : 7 novembre 2026",
 lg_eyebrow="Mentions légales", lg_h1="Mentions légales.", lg_lede="Pour le public, les partenaires et les autorités.",
 lg_who="Éditeur",
 lg_k=["Nom","Forme juridique","Statut","Numéro d'entreprise","Localisation","Fondateur","Cofondatrice","Contact","Site web","Hébergement"],
 lg_v=["Aether EdTech","Association sans but lucratif (ASBL), de droit belge","<strong>En cours d'enregistrement</strong> à Bruxelles","En attente (BCE)","Schaerbeek, Bruxelles, Belgique","Taqi Abbas","Minha Shahid","<a href='mailto:"+EMAIL+"'>"+EMAIL+"</a>","myaether.org","Serveurs situés dans l'Union européenne"],
 lg_after="Cette page sera complétée avec l'adresse officielle, le numéro d'entreprise et un lien vers les statuts dès que l'enregistrement sera finalisé et publié au <em>Moniteur belge</em>.",
 lg_sections=[
  ("Dons et attestations","Tant que l'enregistrement n'est pas finalisé, Aether EdTech ne peut pas émettre d'attestations fiscales pour les dons, et aucun donateur ne doit s'attendre à une déduction fiscale. Tout changement sera annoncé sur ce site."),
  ("Contenu et propriété intellectuelle","Les textes, le design et le code de ce site appartiennent à Aether EdTech, sauf mention contraire. Vous pouvez citer de courts extraits avec un lien vers la source. BacMath.eu et les autres projets liés ont leurs propres conditions."),
  ("Responsabilité","Nous veillons à l'exactitude de ce site, mais ne pouvons garantir l'absence d'erreurs. Nous ne sommes pas responsables du contenu des sites externes vers lesquels nous renvoyons."),
  ("Marques","Google, Microsoft, GoodStack, Adobe, Canva et StartLab Brussels ne sont cités ici que pour les remercier de leur soutien. Leurs noms sont des marques de leurs propriétaires respectifs et n'impliquent aucune approbation d'Aether EdTech."),
  ("Protection des données","Voir notre <a href='privacy.html'>politique de confidentialité et informations RGPD</a> et notre <a href='cookies.html'>déclaration sur les cookies</a>."),
  ("Droit applicable","Le droit belge s'applique. Les tribunaux de Bruxelles sont compétents, sans préjudice des droits impératifs des consommateurs."),
 ],
 pv_eyebrow="Confidentialité &amp; RGPD", pv_h1="Politique de confidentialité et informations RGPD.",
 pv_lede="En bref : nous ne vous suivons pas, nous n'utilisons aucun cookie, et vos données restent dans l'Union européenne.",
 pv_sections=[
  ("En bref","<ul><li>Aucun cookie, aucune statistique, aucune publicité, aucun pixel de suivi.</li><li>Nous ne collectons aucune donnée sur votre visite, et rien n'est vendu ni partagé à des fins commerciales.</li><li>Ce site est hébergé sur des serveurs situés dans l'Union européenne.</li><li>Nous ne transférons aucune donnée personnelle en dehors de l'Espace économique européen.</li><li>Vous pouvez nous écrire à tout moment pour exercer vos droits.</li></ul>"),
  ("Qui est responsable","Le responsable du traitement est <strong>Aether EdTech</strong>, Schaerbeek, Bruxelles, Belgique (enregistrement en cours). Contact : <a href='mailto:"+EMAIL+"'>"+EMAIL+"</a>. Nous ne sommes pas tenus de désigner un délégué à la protection des données ; les questions de confidentialité vont à la même adresse."),
  ("Ce que nous traitons, pourquoi et combien de temps","<h3>Visite du site</h3><p>Nous ne collectons aucune donnée personnelle des visiteurs. Comme tout serveur web, notre hébergeur peut enregistrer techniquement votre adresse IP et l'heure de votre requête dans des journaux de sécurité de courte durée, conservés dans l'UE, afin que le site fonctionne en sécurité. Nous n'utilisons pas ces journaux pour identifier ou profiler les visiteurs.</p><h3>Choix de la langue</h3><p>La langue du site est choisie par un petit script qui lit le réglage de langue de votre navigateur, sur votre propre appareil. Cette information ne nous est pas transmise et rien n'est enregistré.</p><h3>Lorsque vous nous écrivez</h3><p>Nous traitons votre nom, votre adresse e-mail et le contenu de votre message pour vous répondre. Base légale : notre intérêt légitime à répondre aux demandes, ou des mesures précontractuelles prises à votre demande (art. 6, §1, f) et b) RGPD). Nous conservons les messages le temps nécessaire au traitement de votre demande, normalement pas plus de 24 mois après le dernier échange.</p><h3>Dons par virement</h3><p>Votre banque nous communique votre nom, votre numéro de compte et le montant. Nous les conservons pour respecter nos obligations comptables en droit belge (art. 6, §1, c) RGPD), normalement pendant sept ans.</p>"),
  ("Où sont conservées vos données","Notre site et notre messagerie sont exploités dans l'Union européenne. Nous choisissons des prestataires qui traitent les données dans l'UE/EEE et n'agissent que sur nos instructions. Nous ne transférons aucune donnée personnelle vers des pays hors de l'Espace économique européen. Si cela devait changer, nous vous en informerions d'abord et n'utiliserions que les garanties exigées par le RGPD."),
  ("Vos droits au titre du RGPD","<p>Vous avez le droit de :</p><ul><li>accéder aux données personnelles que nous détenons sur vous ;</li><li>faire rectifier les données inexactes ;</li><li>faire effacer vos données (« droit à l'oubli ») ;</li><li>limiter le traitement ou vous y opposer ;</li><li>recevoir vos données dans un format portable ;</li><li>retirer votre consentement à tout moment, lorsque le traitement repose sur celui-ci.</li></ul><p>Écrivez à <a href='mailto:"+EMAIL+"'>"+EMAIL+"</a>. Nous répondons dans un délai d'un mois. Nous pouvons vous demander de confirmer votre identité.</p>"),
  ("Droit de réclamation","Si vous estimez que nous traitons vos données illégalement, vous pouvez introduire une plainte auprès de l'Autorité de protection des données : Rue de la Presse 35, 1000 Bruxelles, <a href='mailto:contact@apd-gba.be'>contact@apd-gba.be</a>, <a href='https://www.autoriteprotectiondonnees.be' rel='noopener'>autoriteprotectiondonnees.be</a>. Nous apprécierions de pouvoir d'abord corriger la situation."),
  ("Enfants","Nous ne collectons pas sciemment de données personnelles d'enfants via ce site. BacMath.eu est un site distinct, avec ses propres informations de confidentialité."),
  ("Liens vers d'autres sites","Les liens vers BacMath.eu et d'autres sites mènent à des services que nous ne contrôlons pas pour ce site. Leurs propres politiques s'appliquent dès que vous quittez celui-ci."),
  ("Sécurité","Nous limitons l'accès à la messagerie aux fondateurs et utilisons des prestataires reconnus, établis dans l'UE. Aucune méthode n'est parfaitement sûre ; nous vous informerons, ainsi que l'autorité, si une violation vous concerne, comme l'exige le RGPD."),
  ("Modifications","Nous mettrons cette page à jour si nos pratiques changent et indiquerons la date en haut. Les changements importants seront annoncés sur la page d'accueil."),
 ],
 ck_eyebrow="Cookies", ck_h1="Déclaration sur les cookies.", ck_lede="Ce site n'utilise aucun cookie.",
 ck_sections=[
  ("Ce que nous utilisons","Rien. Nous ne déposons aucun cookie et n'utilisons ni stockage local, ni empreinte numérique, ni statistiques, ni publicité, ni widgets de réseaux sociaux."),
  ("Polices et ressources","Les polices, images et scripts sont servis par ce site lui-même. Nous ne les chargeons pas depuis des tiers : votre navigateur ne contacte donc aucune autre entreprise lorsque vous nous visitez."),
  ("Si cela change","Si nous ajoutons un jour quelque chose qui nécessite votre consentement, nous le demanderons avant le chargement, comme l'exigent le droit belge et européen, et nous mettrons cette page à jour."),
  ("Plus d'informations","Voir la <a href='privacy.html'>politique de confidentialité et informations RGPD</a>."),
 ],
)

S["nl"] = dict(
 skip="Naar de inhoud", menu="Menu", langlabel="Taal",
 n_index="Home", n_about="Over ons", n_projects="Projecten", n_services="Diensten", n_contact="Contact", n_support="Steun ons",
 f_tag="Een vereniging zonder winstoogmerk voor onderwijs en educatieve projecten, in Schaarbeek, Brussel.",
 f_explore="Ontdek", f_project="Project", f_partner="Word partner", f_reach="Contact", f_legal="Juridisch",
 f_status="<strong>Juridisch statuut:</strong> Aether EdTech is een vereniging zonder winstoogmerk (vzw / ASBL) die momenteel in Brussel wordt geregistreerd. Ondernemingsnummer (KBO) in afwachting. Zolang de registratie niet is afgerond, geven we geen fiscale attesten uit. <a href='legal.html'>Details</a>",
 f_eu="Gehost in de EU · Geen cookies · Geen tracking",
 t_index="Aether EdTech · Onderwijsvereniging in Brussel",
 d_index="Aether EdTech is een vereniging zonder winstoogmerk (vzw/ASBL) in Schaarbeek, Brussel, die onderwijs en educatieve projecten ondersteunt. Makers van BacMath.eu.",
 t_about="Over ons · Aether EdTech", d_about="Maak kennis met de oprichters van Aether EdTech, een Brusselse onderwijsvereniging: Taqi Abbas en Minha Shahid. Ons doel, onze waarden en ons statuut.",
 t_projects="Projecten · Aether EdTech", d_projects="BacMath.eu, de gratis leerwebsite van Aether EdTech, en de andere educatieve initiatieven waaraan we meewerken.",
 t_services="Diensten · Aether EdTech", d_services="Softwareontwikkeling en digitaal advies van een Brusselse vereniging zonder winstoogmerk. Overschot financiert gratis educatieve projecten.",
 t_support="Steun ons · Aether EdTech", d_support="Doe een gift, help mee of word partner van Aether EdTech om gratis educatieve projecten draaiende te houden.",
 t_contact="Contact · Aether EdTech", d_contact="Neem contact op met Aether EdTech in Schaarbeek, Brussel, over giften, partnerschappen, vrijwilligerswerk of softwareprojecten.",
 t_legal="Juridische kennisgeving · Aether EdTech", d_legal="Juridisch statuut, stand van de registratie en uitgever van Aether EdTech, Brussel.",
 t_privacy="Privacybeleid en AVG · Aether EdTech", d_privacy="Hoe Aether EdTech uw gegevens beschermt: geen tracking, hosting in de EU, uw AVG-rechten en hoe u ze uitoefent.",
 t_cookies="Cookieverklaring · Aether EdTech", d_cookies="Deze website gebruikt geen cookies en geen trackingtechnologieën.",
 h_eyebrow="Vereniging zonder winstoogmerk · Onderwijs · Brussel",
 h_h1="Goed leren moet <span class='scribble'>voor iedereen bereikbaar</span> zijn, waar je ook begint.",
 h_lede="Aether EdTech is een Belgische vereniging zonder winstoogmerk (vzw/ASBL) die onderwijs en educatieve projecten ondersteunt. We bouwen gratis leermiddelen, werken mee aan initiatieven die onze zaak delen en financieren dit werk met eerlijke softwarediensten die onze missie dienen.",
 h_b1="Probeer BacMath.eu", h_b2="Steun ons werk",
 h_pill="Aether EdTech · Schaarbeek, Brussel · registratie lopende",
 id_h="In het kort", id1="Gevestigd in", id1v="Schaarbeek, Brussel, België", id2="Statuut", id2v="vzw/ASBL, registratie lopende",
 id3="Lopend project", id3v="BacMath.eu", id4="Oprichters", id4v="Taqi Abbas &amp; Minha Shahid",
 w_eyebrow="Wat we doen", w_h2="Drie manieren waarop we voor onderwijs werken.", w_lede="We zijn leerkrachten en ingenieurs. Die mix bepaalt alles wat we maken.",
 r1h="We bouwen leermiddelen", r1p="Websites en software met de klas in gedachten: helder, speels en gratis voor de leerling. BacMath.eu is de eerste, en vandaag al online.",
 r2h="We sluiten aan bij andere initiatieven", r2p="Onderwijs verbeter je zelden alleen. We brengen vaardigheden, software en tijd in bij projecten van scholen, verenigingen en buurtgroepen.",
 r3h="We financieren het met onze diensten", r3p="We bouwen software voor scholen, organisaties en bedrijven. Elk overschot gaat terug naar gratis educatieve projecten, want een vzw mag geen winst uitkeren.",
 fe_eyebrow="Ons lopende project",
 fe_p1="Een leerwebsite voor wiskunde die oefenen een spel maakt in plaats van een straf. Leerlingen gaan in hun eigen tempo, in een vorm waardoor ze morgen graag terugkomen.",
 fe_p2="Het is gratis te gebruiken, en dat blijft zo dankzij mensen zoals u die ons helpen het draaiende te houden.",
 fe_t1="Wiskunde leuk en toegankelijk uitgelegd", fe_t2="Open voor elke leerling, gratis om te starten", fe_t3="Gebouwd en onderhouden door leerkrachten en ingenieurs",
 fe_b1="Bezoek BacMath.eu", fe_b2="Over het project",
 fe_q="Leer het.<br>Speel ermee.<br>Onthoud het.", fe_cap="BacMath.eu",
 th_eyebrow="Met dank aan", th_h2="We doen dit niet alleen.", th_lede="Aether EdTech is jong, en deze organisaties steunden het al. Dank u wel.",
 th_fine="Namen worden als tekst getoond om hun steun te erkennen. Het zijn handelsmerken van hun respectieve eigenaars en deze vermelding impliceert geen goedkeuring van Aether EdTech.",
 b_h2="Help ons leren gratis te houden.", b_p="Giften betalen hosting, ontwikkeltijd en nieuwe lessen. Kunt u geen geld geven, dan kunt u vaardigheden, tijd of een introductie geven. Alles telt.", b_btn="Zie hoe u kunt helpen",
 ab_eyebrow="Over Aether EdTech", ab_h1="Twee leerkrachten, één codebase, één doel.",
 ab_lede="Aether EdTech begon met een eenvoudige vaststelling: veel goed onderwijs komt nooit verder dan het lokaal waar het gebeurt. Dat willen we veranderen, met software die het verder brengt.",
 c_eyebrow="Ons doel", c_h2="Onderwijs dat niet afhangt van waar je geboren bent.",
 c_p1="Aether EdTech bestaat om onderwijs en educatieve projecten te ondersteunen. Dat is een brede zin, dus hier wat het in de praktijk betekent.",
 c_p2="We maken leermiddelen die gratis zijn voor de leerling. We bouwen en onderhouden de software erachter. En we werken mee aan andere initiatieven, met technische en pedagogische vaardigheden voor mensen die al goed werk leveren en een extra paar handen kunnen gebruiken.",
 bel_h="Waar we in geloven",
 bel1="<strong>Leren moet leuk zijn.</strong> Een nieuwsgierige leerling leert meer dan een bange.",
 bel2="<strong>Toegang mag geen voorrecht zijn.</strong> Essentiële educatieve hulpmiddelen blijven gratis.",
 bel3="<strong>Eerlijk zijn over wie we zijn.</strong> We zijn klein, we zijn nieuw, en we zeggen dat.",
 bel4="<strong>Herinvesteren, nooit onttrekken.</strong> Elk overschot gaat terug naar het educatieve werk.",
 fo_eyebrow="Oprichters", fo_h2="De mensen erachter.",
 p1_role="Oprichter", p1_prof="Onderwijskundige · Leerkracht · Software-ingenieur",
 p1_bio="Taqi bouwt de tools en houdt de projecten draaiende, met een klasperspectief op wat echt nuttig is.",
 p2_role="Medeoprichter", p2_prof="Onderwijskundige · Leerkracht · Arts",
 p2_bio="Minha brengt de blik van een leerkracht op hoe mensen leren, en de zorg die bij het werk in de geneeskunde hoort.",
 wh_h="Waar we zijn", wh_p="Schaarbeek, Brussel, België. Deze website is beschikbaar in het Engels, Frans, Nederlands en Duits.",
 st_h="Ons juridisch statuut",
 st_p="<strong>Registratie lopende.</strong> Aether EdTech wordt in Brussel opgericht als <em>vereniging zonder winstoogmerk</em> (vzw/ASBL). We publiceren het ondernemingsnummer en de statuten zodra de registratie is afgerond.",
 pr_eyebrow="Projecten", pr_h1="Wat we bouwen, en waar we bij aansluiten.",
 pr_lede="Gratis educatieve middelen zijn ons algemeen goed. Ze staan open voor iedereen en komen eerst.",
 pr_live="Nu online",
 pr_p1="BacMath.eu is een educatieve wiskundewebsite die leren leuk maakt. Het is ons belangrijkste lopende project en het duidelijkste beeld van waarvoor Aether EdTech bestaat: een vak waar veel leerlingen tegen opzien, iets maken waar ze plezier aan beleven.",
 pr_p2="Het is gratis te gebruiken. Hosting, ontwikkeling en nieuwe inhoud kosten tijd en geld, daarom is uw steun belangrijk.",
 pr_b1="Open BacMath.eu", pr_b2="Help het financieren", pr_q="Leer het.<br>Speel ermee.<br>Onthoud het.", pr_cap="Het idee achter BacMath",
 ot_eyebrow="Voorbij BacMath", ot_h2="Andere initiatieven.",
 ot_p1="We werken mee aan initiatieven die onze zaak delen. Soms bouwen we een tool voor een school of vereniging. Soms geven we lestijd, advies of een extra ontwikkelaar voor een paar weken.",
 ot_p2="We vermelden partnerprojecten hier zodra ze bevestigd zijn. We tonen liever een korte, ware lijst dan een lange, opgevulde.",
 ot_h3="Een project in gedachten?", ot_p3="Leidt u een educatief project en hebt u softwarevaardigheden nodig, of geeft u les en hebt u een idee dat gebouwd moet worden? Vertel het ons.", ot_btn="Start een gesprek",
 sv_eyebrow="Diensten", sv_h1="Software op maat, met een doel.",
 sv_lede="We bouwen websites en software voor scholen, verenigingen en bedrijven. Wat na de kosten overblijft, financiert gratis educatieve projecten zoals BacMath.eu.",
 sv_how="Hoe het werkt", sv_h2="Twee kanten van één organisatie.",
 sv_f_h="Gratis, altijd", sv_f_p="Onze publieke educatieve middelen, te beginnen met BacMath.eu. Open voor elke leerling, gefinancierd door giften, partners en de diensten op deze pagina.",
 sv_p_h="Betaald, met een reden", sv_p_p="Softwareprojecten voor organisaties die ze nodig hebben. Vergoedingen dekken onze kosten en elk overschot wordt herïnvesteerd in onderwijs, zoals de wet voor een vzw vereist.",
 sv1h="Websites en webtoepassingen", sv1p="Heldere, snelle, toegankelijke sites en tools voor scholen, verenigingen en kleine organisaties.",
 sv2h="Educatieve software", sv2p="Leerplatformen, quizzen, oefentools en contentsystemen, gebouwd met inbreng van een leerkracht vanaf de eerste schets.",
 sv3h="Digitaal advies", sv3p="Praktische begeleiding voor instellingen bij het kiezen van tools, online gaan en digitale diensten gebruiksvriendelijker maken.",
 sv_b_h="Bestel iets dat dubbel goed doet.", sv_b_p="U krijgt de software die u nodig hebt. Onderwijs krijgt het overschot. Vertel ons wat u van plan bent.", sv_b_btn="Neem contact op",
 su_eyebrow="Steun ons", su_h1="Leren binnen handbereik houden.",
 su_lede="We zijn een klein team met een duidelijke taak. Geld, vaardigheden en introducties helpen ons het langer vol te houden.",
 w1h="Doe een gift", w1p="Uw gift betaalt hosting, ontwikkeltijd en nieuwe lessen. Onze bankrekening wordt afgerond als onderdeel van de registratie. Wilt u nu geven, schrijf ons dan en we sturen de overschrijvingsgegevens.", w1a="Vraag de gegevens voor een gift",
 w2h="Geef vaardigheden of tijd", w2p="Leerkrachten, ontwikkelaars, ontwerpers, vertalers: als u een paar uur kunt geven, is er waarschijnlijk een taak voor u.", w2a="Bied uw vaardigheden aan",
 w3h="Word partner", w3p="Bedrijven en stichtingen kunnen een project sponsoren, tools leveren of onze diensten bestellen, wetend dat het overschot naar onderwijs gaat.", w3a="Praat over partnerschap",
 w4h="Verspreid het woord", w4p="Deel BacMath.eu met een leerling, ouder of leerkracht. Het kost niets en zo vinden de meeste mensen ons.", w4a="Deel BacMath.eu",
 don_subj="Gift aan Aether EdTech",
 sg_eyebrow="Eerlijk gezegd", sg_h2="Wat u moet weten voor u geeft.", sg_p="We vertellen het u liever zelf dan dat u het later ontdekt.",
 sg1="<strong>We zijn nog niet geregistreerd.</strong> Aether EdTech is bezig met de registratie in Brussel. We kondigen dat aan op deze site.",
 sg2="<strong>Voorlopig geen fiscaal attest.</strong> In België vereisen fiscaal aftrekbare attesten een formele erkenning. Die hebben we niet en we zullen ze niet claimen. Als dat verandert, zeggen we het.",
 sg3="<strong>Waar het geld naartoe gaat.</strong> Hosting, softwaretools, ontwikkeling en educatieve inhoud voor onze projecten. Na registratie publiceren we een eenvoudig jaaroverzicht.",
 so_eyebrow="Onze steuners", so_h2="Dank u wel.",
 so_note="Bijzondere dank aan <strong>StartLab Brussels</strong>, wiens steun ons helpt om in Brussel van start te gaan.",
 ct_eyebrow="Contact", ct_h1="Schrijf ons.", ct_lede="Vragen, ideeën, hulpaanbiedingen, offerteaanvragen: alles komt in dezelfde inbox, en een echt persoon antwoordt.",
 ct_email="E-mail", ct_place="Plaats", ct_project="Project", ct_status="Statuut", ct_status_v="vzw/ASBL-registratie lopende",
 ct_name="Uw naam", ct_mail="Uw e-mail", ct_topic="Ik schrijf over", ct_msg="Bericht",
 ct_o1="Een gift", ct_o2="Vaardigheden of tijd aanbieden", ct_o3="Een partnerschap", ct_o4="Een softwareproject", ct_o5="BacMath.eu", ct_o6="Iets anders",
 ct_btn="Openen in mijn e-mailprogramma", ct_fine="Dit opent uw eigen e-mailprogramma met het bericht klaar om te verzenden. Er wordt niets naar deze website gestuurd of erop bewaard. Zie ons <a href='privacy.html'>privacybeleid</a>.",
 upd="Laatst bijgewerkt: 7 november 2026",
 lg_eyebrow="Juridische kennisgeving", lg_h1="Juridische kennisgeving.", lg_lede="Voor het publiek, partners en de overheid.",
 lg_who="Uitgever",
 lg_k=["Naam","Rechtsvorm","Statuut","Ondernemingsnummer","Locatie","Oprichter","Medeoprichter","Contact","Website","Hosting"],
 lg_v=["Aether EdTech","Vereniging zonder winstoogmerk (vzw / ASBL), naar Belgisch recht","<strong>Registratie lopende</strong> in Brussel","In afwachting (KBO)","Schaarbeek, Brussel, België","Taqi Abbas","Minha Shahid","<a href='mailto:"+EMAIL+"'>"+EMAIL+"</a>","myaether.org","Servers in de Europese Unie"],
 lg_after="Deze pagina wordt aangevuld met het volledige maatschappelijke adres, het ondernemingsnummer en een link naar de statuten zodra de registratie is afgerond en gepubliceerd in het <em>Belgisch Staatsblad</em>.",
 lg_sections=[
  ("Giften en attesten","Zolang de registratie niet is afgerond, kan Aether EdTech geen fiscale attesten voor giften uitreiken en mag geen schenker een belastingaftrek verwachten. Elke wijziging kondigen we op deze site aan."),
  ("Inhoud en intellectuele eigendom","Teksten, ontwerp en code van deze website zijn eigendom van Aether EdTech, tenzij anders vermeld. U mag korte fragmenten citeren met een link naar de bron. BacMath.eu en andere gelinkte projecten hebben eigen voorwaarden."),
  ("Aansprakelijkheid","We zorgen dat deze site juist is, maar kunnen niet garanderen dat ze vrij is van fouten. We zijn niet verantwoordelijk voor de inhoud van externe sites waarnaar we linken."),
  ("Handelsmerken","Google, Microsoft, GoodStack, Adobe, Canva en StartLab Brussels worden hier enkel genoemd om hen te bedanken voor hun steun. Hun namen zijn handelsmerken van hun respectieve eigenaars en impliceren geen goedkeuring van Aether EdTech."),
  ("Gegevensbescherming","Zie ons <a href='privacy.html'>privacybeleid en AVG-informatie</a> en onze <a href='cookies.html'>cookieverklaring</a>."),
  ("Toepasselijk recht","Het Belgisch recht is van toepassing. De rechtbanken van Brussel zijn bevoegd, onverminderd de dwingende rechten van consumenten."),
 ],
 pv_eyebrow="Privacy &amp; AVG", pv_h1="Privacybeleid en AVG-informatie.",
 pv_lede="Kort gezegd: we volgen u niet, we gebruiken geen cookies en uw gegevens blijven in de Europese Unie.",
 pv_sections=[
  ("In het kort","<ul><li>Geen cookies, geen analytics, geen reclame, geen trackingpixels.</li><li>Wij verzamelen geen gegevens over uw bezoek en niets wordt verkocht of gedeeld voor marketing.</li><li>Deze website wordt gehost op servers in de Europese Unie.</li><li>We geven geen persoonsgegevens door buiten de Europese Economische Ruimte.</li><li>U kunt ons op elk moment mailen om uw rechten uit te oefenen.</li></ul>"),
  ("Wie verantwoordelijk is","De verwerkingsverantwoordelijke is <strong>Aether EdTech</strong>, Schaarbeek, Brussel, België (registratie lopende). Contact: <a href='mailto:"+EMAIL+"'>"+EMAIL+"</a>. We zijn niet verplicht een functionaris voor gegevensbescherming aan te stellen; privacyvragen gaan naar hetzelfde adres."),
  ("Wat we verwerken, waarom en hoe lang","<h3>Bezoek aan deze website</h3><p>We verzamelen geen persoonsgegevens van bezoekers. Zoals elke webserver kan onze hostingprovider technisch uw IP-adres en het tijdstip van uw verzoek vastleggen in kortlevende beveiligingslogs, bewaard in de EU, zodat de site veilig kan draaien. We gebruiken deze logs niet om bezoekers te identificeren of te profileren.</p><h3>Taalkeuze</h3><p>De taal van de site wordt gekozen door een klein script dat de taalinstelling van uw browser leest, op uw eigen apparaat. Deze informatie wordt niet naar ons gestuurd en er wordt niets opgeslagen.</p><h3>Wanneer u ons mailt</h3><p>We verwerken uw naam, e-mailadres en de inhoud van uw bericht om u te antwoorden. Rechtsgrond: ons gerechtvaardigd belang om vragen te beantwoorden, of stappen die u vraagt vóór een overeenkomst (art. 6, lid 1, f) en b) AVG). We bewaren berichten zolang nodig voor de afhandeling, normaal niet langer dan 24 maanden na de laatste uitwisseling.</p><h3>Giften via overschrijving</h3><p>Uw bank bezorgt ons uw naam, rekeningnummer en het bedrag. We bewaren dit om te voldoen aan onze boekhoudkundige verplichtingen naar Belgisch recht (art. 6, lid 1, c) AVG), normaal zeven jaar.</p>"),
  ("Waar uw gegevens worden bewaard","Onze website en mailbox worden beheerd binnen de Europese Unie. We kiezen providers die gegevens verwerken in de EU/EER en enkel op onze instructies handelen. We geven geen persoonsgegevens door aan landen buiten de Europese Economische Ruimte. Als dat ooit verandert, vertellen we het u eerst en gebruiken we enkel de waarborgen die de AVG vereist."),
  ("Uw rechten onder de AVG","<p>U hebt het recht om:</p><ul><li>de persoonsgegevens in te zien die we over u bewaren;</li><li>onjuiste gegevens te laten verbeteren;</li><li>uw gegevens te laten wissen (&ldquo;recht om vergeten te worden&rdquo;);</li><li>de verwerking te beperken of u ertegen te verzetten;</li><li>uw gegevens in een overdraagbaar formaat te ontvangen;</li><li>uw toestemming op elk moment in te trekken, wanneer de verwerking daarop berust.</li></ul><p>Schrijf naar <a href='mailto:"+EMAIL+"'>"+EMAIL+"</a>. We antwoorden binnen één maand. We kunnen u vragen uw identiteit te bevestigen.</p>"),
  ("Recht om een klacht in te dienen","Als u denkt dat we uw gegevens onrechtmatig verwerken, kunt u een klacht indienen bij de Gegevensbeschermingsautoriteit: Drukpersstraat 35, 1000 Brussel, <a href='mailto:contact@apd-gba.be'>contact@apd-gba.be</a>, <a href='https://www.autoriteprotectiondonnees.be' rel='noopener'>gegevensbeschermingsautoriteit.be</a>. We waarderen het als we eerst de kans krijgen het recht te zetten."),
  ("Kinderen","We verzamelen via deze website niet bewust persoonsgegevens van kinderen. BacMath.eu is een aparte website met eigen privacyinformatie."),
  ("Links naar andere websites","Links naar BacMath.eu en andere sites leiden naar diensten die we voor deze website niet beheren. Hun eigen privacybeleid geldt zodra u deze site verlaat."),
  ("Beveiliging","We beperken de toegang tot de mailbox tot de oprichters en gebruiken gerenommeerde providers in de EU. Geen enkele methode is volkomen veilig; we brengen u en de autoriteit op de hoogte als een datalek u treft, zoals de AVG vereist."),
  ("Wijzigingen","We werken deze pagina bij als onze werkwijze verandert en tonen de datum bovenaan. Belangrijke wijzigingen kondigen we aan op de homepage."),
 ],
 ck_eyebrow="Cookies", ck_h1="Cookieverklaring.", ck_lede="Deze website gebruikt geen cookies.",
 ck_sections=[
  ("Wat we gebruiken","Niets. We plaatsen geen cookies en gebruiken geen lokale opslag, fingerprinting, analytics, reclame of social-mediawidgets."),
  ("Lettertypen en bronnen","Lettertypen, afbeeldingen en scripts worden door deze website zelf geleverd. We laden ze niet van derden, zodat uw browser geen andere bedrijven contacteert wanneer u ons bezoekt."),
  ("Als dit verandert","Als we ooit iets toevoegen dat uw toestemming vereist, vragen we die vóór het laadt, zoals het Belgische en Europese recht vereist, en passen we deze pagina aan."),
  ("Meer informatie","Zie het <a href='privacy.html'>privacybeleid en AVG-informatie</a>."),
 ],
)

S["de"] = dict(
 skip="Zum Inhalt springen", menu="Menü", langlabel="Sprache",
 n_index="Start", n_about="Über uns", n_projects="Projekte", n_services="Leistungen", n_contact="Kontakt", n_support="Unterstützen",
 f_tag="Eine gemeinnützige Vereinigung für Bildung und Bildungsprojekte, in Schaerbeek, Brüssel.",
 f_explore="Entdecken", f_project="Projekt", f_partner="Partner werden", f_reach="Kontakt", f_legal="Rechtliches",
 f_status="<strong>Rechtsstatus:</strong> Aether EdTech ist eine Vereinigung ohne Gewinnerzielungsabsicht (VoG / ASBL), die derzeit in Brüssel eingetragen wird. Unternehmensnummer (ZDU) ausstehend. Bis zum Abschluss der Eintragung stellen wir keine Steuerbescheinigungen aus. <a href='legal.html'>Details</a>",
 f_eu="Gehostet in der EU · Keine Cookies · Kein Tracking",
 t_index="Aether EdTech · Bildungsverein in Brüssel",
 d_index="Aether EdTech ist eine gemeinnützige Vereinigung (VoG/ASBL) in Schaerbeek, Brüssel, die Bildung und Bildungsprojekte unterstützt. Entwickler von BacMath.eu.",
 t_about="Über uns · Aether EdTech", d_about="Die Gründer von Aether EdTech, einem Brüsseler Bildungsverein: Taqi Abbas und Minha Shahid. Unser Anliegen, unsere Werte, unser Rechtsstatus.",
 t_projects="Projekte · Aether EdTech", d_projects="BacMath.eu, die kostenlose Lernwebsite von Aether EdTech, und die weiteren Bildungsinitiativen, an denen wir beteiligt sind.",
 t_services="Leistungen · Aether EdTech", d_services="Softwareentwicklung und digitale Beratung von einem Brüsseler gemeinnützigen Verein. Überschüsse finanzieren kostenlose Bildungsprojekte.",
 t_support="Unterstützen · Aether EdTech", d_support="Spenden, mithelfen oder Partner von Aether EdTech werden, damit kostenlose Bildungsprojekte weiterlaufen.",
 t_contact="Kontakt · Aether EdTech", d_contact="Kontakt zu Aether EdTech in Schaerbeek, Brüssel, für Spenden, Partnerschaften, Ehrenamt oder Softwareprojekte.",
 t_legal="Impressum · Aether EdTech", d_legal="Rechtsstatus, Stand der Eintragung und Herausgeber von Aether EdTech, Brüssel.",
 t_privacy="Datenschutzerklärung und DSGVO · Aether EdTech", d_privacy="Wie Aether EdTech Ihre Daten schützt: kein Tracking, Hosting in der EU, Ihre DSGVO-Rechte und wie Sie sie ausüben.",
 t_cookies="Cookie-Erklärung · Aether EdTech", d_cookies="Diese Website verwendet keine Cookies und keine Tracking-Technologien.",
 h_eyebrow="Gemeinnützig · Bildung · Brüssel",
 h_h1="Gutes Lernen soll <span class='scribble'>für alle erreichbar</span> sein, egal wo man beginnt.",
 h_lede="Aether EdTech ist eine belgische gemeinnützige Vereinigung (VoG/ASBL), die Bildung und Bildungsprojekte unterstützt. Wir entwickeln kostenlose Lernangebote, beteiligen uns an Initiativen, die unser Anliegen teilen, und finanzieren die Arbeit durch ehrliche, dem Auftrag verpflichtete Softwaredienste.",
 h_b1="BacMath.eu ausprobieren", h_b2="Unsere Arbeit unterstützen",
 h_pill="Aether EdTech · Schaerbeek, Brüssel · Eintragung läuft",
 id_h="Auf einen Blick", id1="Sitz", id1v="Schaerbeek, Brüssel, Belgien", id2="Status", id2v="VoG/ASBL, Eintragung läuft",
 id3="Laufendes Projekt", id3v="BacMath.eu", id4="Gründer", id4v="Taqi Abbas &amp; Minha Shahid",
 w_eyebrow="Was wir tun", w_h2="Drei Wege, wie wir für Bildung arbeiten.", w_lede="Wir sind Lehrkräfte und Ingenieure. Diese Mischung prägt alles, was wir bauen.",
 r1h="Wir entwickeln Lernangebote", r1p="Websites und Software mit Blick auf den Unterricht: klar, spielerisch und kostenlos für Lernende. BacMath.eu ist das erste und schon heute online.",
 r2h="Wir beteiligen uns an anderen Initiativen", r2p="Bildung verbessert selten eine Organisation allein. Wir bringen Können, Software und Zeit in Projekte von Schulen, Vereinen und Bürgergruppen ein.",
 r3h="Wir finanzieren es durch Dienstleistungen", r3p="Wir entwickeln Software für Schulen, Organisationen und Unternehmen. Jeder Überschuss fließt zurück in kostenlose Bildungsprojekte, denn eine VoG darf keinen Gewinn ausschütten.",
 fe_eyebrow="Unser laufendes Projekt",
 fe_p1="Eine Lernwebsite für Mathematik, die Üben zum Spiel macht statt zur Strafe. Lernende kommen in ihrem eigenen Tempo voran, in einer Form, die Lust macht, morgen wiederzukommen.",
 fe_p2="Sie ist kostenlos nutzbar und bleibt es, weil Menschen wie Sie uns helfen, sie am Laufen zu halten.",
 fe_t1="Mathematik unterhaltsam und verständlich erklärt", fe_t2="Offen für alle Lernenden, kostenlos zum Start", fe_t3="Entwickelt und gepflegt von Lehrkräften und Ingenieuren",
 fe_b1="BacMath.eu besuchen", fe_b2="Über das Projekt",
 fe_q="Lernen.<br>Spielen.<br>Behalten.", fe_cap="BacMath.eu",
 th_eyebrow="Mit Dank an", th_h2="Wir sind nicht allein.", th_lede="Aether EdTech ist jung, und diese Organisationen haben es bereits unterstützt. Wir sind dankbar.",
 th_fine="Namen als Text zur Würdigung der Unterstützung. Es sind Marken ihrer jeweiligen Inhaber; diese Nennung bedeutet keine Billigung von Aether EdTech.",
 b_h2="Helfen Sie uns, Lernen kostenlos zu halten.", b_p="Spenden bezahlen Hosting, Entwicklungszeit und neue Lektionen. Wenn Sie kein Geld geben können, können Sie Können, Zeit oder eine Vermittlung geben. Alles zählt.", b_btn="So können Sie helfen",
 ab_eyebrow="Über Aether EdTech", ab_h1="Zwei Lehrkräfte, eine Codebasis, ein Anliegen.",
 ab_lede="Aether EdTech begann mit einer einfachen Beobachtung: Viel guter Unterricht kommt nie über den Raum hinaus, in dem er stattfindet. Das wollen wir ändern, mit Software, die ihn weiterträgt.",
 c_eyebrow="Unser Anliegen", c_h2="Bildung, die nicht davon abhängt, wo man geboren wurde.",
 c_p1="Aether EdTech besteht, um Bildung und Bildungsprojekte zu unterstützen. Das ist ein weiter Satz, daher hier, was er in der Praxis bedeutet.",
 c_p2="Wir schaffen Lernangebote, die für Lernende kostenlos sind. Wir entwickeln und pflegen die Software dahinter. Und wir beteiligen uns an anderen Initiativen und stellen technisches und pädagogisches Können Menschen zur Verfügung, die bereits gute Arbeit leisten und ein weiteres Paar Hände gebrauchen können.",
 bel_h="Woran wir glauben",
 bel1="<strong>Lernen soll Freude machen.</strong> Wer neugierig ist, lernt mehr als wer Angst hat.",
 bel2="<strong>Zugang darf kein Privileg sein.</strong> Zentrale Bildungsangebote bleiben kostenlos.",
 bel3="<strong>Ehrlich sagen, was wir sind.</strong> Wir sind klein, wir sind neu, und wir sagen das.",
 bel4="<strong>Reinvestieren, nie entnehmen.</strong> Jeder Überschuss fließt zurück in die Bildungsarbeit.",
 fo_eyebrow="Gründer", fo_h2="Die Menschen dahinter.",
 p1_role="Gründer", p1_prof="Pädagoge · Lehrer · Software-Ingenieur",
 p1_bio="Taqi entwickelt die Werkzeuge und hält die Projekte am Laufen, mit dem Blick aus dem Klassenzimmer darauf, was wirklich nützt.",
 p2_role="Mitgründerin", p2_prof="Pädagogin · Lehrerin · Ärztin",
 p2_bio="Minha bringt den Blick einer Lehrerin dafür mit, wie Menschen lernen, und die Sorgfalt, die zur Arbeit in der Medizin gehört.",
 wh_h="Wo wir sind", wh_p="Schaerbeek, Brüssel, Belgien. Diese Website gibt es auf Englisch, Französisch, Niederländisch und Deutsch.",
 st_h="Unser Rechtsstatus",
 st_p="<strong>Eintragung läuft.</strong> Aether EdTech wird in Brüssel als <em>Vereinigung ohne Gewinnerzielungsabsicht</em> (VoG/ASBL) gegründet. Wir veröffentlichen Unternehmensnummer und Satzung, sobald die Eintragung abgeschlossen ist.",
 pr_eyebrow="Projekte", pr_h1="Was wir bauen und wo wir mitmachen.",
 pr_lede="Kostenlose Bildungsangebote sind unser Gemeingut. Sie stehen allen offen und haben Vorrang.",
 pr_live="Jetzt online",
 pr_p1="BacMath.eu ist eine Bildungswebsite für Mathematik, die Lernen unterhaltsam macht. Es ist unser wichtigstes laufendes Projekt und das klarste Bild davon, wofür Aether EdTech da ist: ein Fach, vor dem sich viele fürchten, zu etwas machen, das Freude bereiten kann.",
 pr_p2="Die Nutzung ist kostenlos. Hosting, Entwicklung und neue Inhalte kosten Zeit und Geld, deshalb ist Ihre Unterstützung wichtig.",
 pr_b1="BacMath.eu öffnen", pr_b2="Bei der Finanzierung helfen", pr_q="Lernen.<br>Spielen.<br>Behalten.", pr_cap="Die Idee hinter BacMath",
 ot_eyebrow="Über BacMath hinaus", ot_h2="Weitere Initiativen.",
 ot_p1="Wir beteiligen uns an Initiativen, die unser Anliegen teilen. Manchmal bauen wir ein Werkzeug für eine Schule oder einen Verein. Manchmal geben wir Unterrichtszeit, Rat oder für ein paar Wochen einen zusätzlichen Entwickler.",
 ot_p2="Partnerprojekte listen wir hier auf, sobald sie bestätigt sind. Wir zeigen lieber eine kurze, wahre Liste als eine lange, aufgeblähte.",
 ot_h3="Ein Projekt im Kopf?", ot_p3="Sie leiten ein Bildungsprojekt und brauchen Softwarekenntnisse, oder Sie unterrichten und haben eine Idee, die gebaut werden muss? Erzählen Sie uns davon.", ot_btn="Gespräch beginnen",
 sv_eyebrow="Leistungen", sv_h1="Software nach Maß, mit Sinn.",
 sv_lede="Wir entwickeln Websites und Software für Schulen, Vereine und Unternehmen. Was nach den Kosten bleibt, finanziert kostenlose Bildungsprojekte wie BacMath.eu.",
 sv_how="So funktioniert es", sv_h2="Zwei Seiten einer Organisation.",
 sv_f_h="Kostenlos, immer", sv_f_p="Unsere öffentlichen Bildungsangebote, beginnend mit BacMath.eu. Offen für alle Lernenden, finanziert durch Spenden, Partner und die Leistungen auf dieser Seite.",
 sv_p_h="Bezahlt, aus gutem Grund", sv_p_p="Softwareprojekte für Organisationen, die sie brauchen. Honorare decken unsere Kosten, und jeder Überschuss wird in Bildung reinvestiert, wie es das Recht für eine VoG verlangt.",
 sv1h="Websites und Webanwendungen", sv1p="Klare, schnelle, barrierearme Websites und Werkzeuge für Schulen, Vereine und kleine Organisationen.",
 sv2h="Bildungssoftware", sv2p="Lernplattformen, Quizze, Übungswerkzeuge und Inhaltssysteme, mit dem Rat einer Lehrkraft ab der ersten Skizze entwickelt.",
 sv3h="Digitale Beratung", sv3p="Praktische Hilfe für Einrichtungen bei der Wahl von Werkzeugen, beim Gang ins Netz und bei benutzerfreundlicheren digitalen Angeboten.",
 sv_b_h="Beauftragen Sie etwas, das doppelt Gutes tut.", sv_b_p="Sie erhalten die Software, die Sie brauchen. Die Bildung erhält den Überschuss. Erzählen Sie uns, was Sie vorhaben.", sv_b_btn="Kontakt aufnehmen",
 su_eyebrow="Unterstützen", su_h1="Lernen in Reichweite halten.",
 su_lede="Wir sind ein kleines Team mit einer klaren Aufgabe. Geld, Können und Kontakte helfen uns, sie länger zu erfüllen.",
 w1h="Spenden", w1p="Ihre Spende bezahlt Hosting, Entwicklungszeit und neue Lektionen. Unser Bankkonto wird im Rahmen der Eintragung eingerichtet. Wenn Sie jetzt geben möchten, schreiben Sie uns, und wir senden die Überweisungsdaten.", w1a="Spendendaten anfragen",
 w2h="Können oder Zeit schenken", w2p="Lehrkräfte, Entwickler, Designer, Übersetzer: Wenn Sie ein paar Stunden geben können, gibt es wahrscheinlich eine Aufgabe für Sie.", w2a="Ihr Können anbieten",
 w3h="Partner werden", w3p="Unternehmen und Stiftungen können ein Projekt fördern, Werkzeuge bereitstellen oder unsere Leistungen beauftragen, im Wissen, dass der Überschuss der Bildung zugutekommt.", w3a="Über Partnerschaft sprechen",
 w4h="Weitersagen", w4p="Teilen Sie BacMath.eu mit einer Schülerin, einem Elternteil oder einer Lehrkraft. Es kostet nichts, und so finden uns die meisten.", w4a="BacMath.eu teilen",
 don_subj="Spende an Aether EdTech",
 sg_eyebrow="Offen gesagt", sg_h2="Was Sie vor dem Spenden wissen sollten.", sg_p="Sie sollen es lieber von uns erfahren als später.",
 sg1="<strong>Wir sind noch nicht eingetragen.</strong> Aether EdTech befindet sich in Brüssel im Eintragungsverfahren. Wir geben es auf dieser Seite bekannt.",
 sg2="<strong>Vorerst keine Steuerbescheinigung.</strong> In Belgien setzen steuerlich absetzbare Bescheinigungen eine förmliche Anerkennung voraus. Wir haben sie nicht und werden sie nicht für uns beanspruchen. Ändert sich das, sagen wir es.",
 sg3="<strong>Wohin das Geld geht.</strong> Hosting, Softwarewerkzeuge, Entwicklung und Bildungsinhalte für unsere Projekte. Nach der Eintragung veröffentlichen wir eine einfache Jahresübersicht.",
 so_eyebrow="Unsere Unterstützer", so_h2="Danke.",
 so_note="Besonderer Dank gilt <strong>StartLab Brussels</strong>, dessen Unterstützung uns beim Start in Brüssel hilft.",
 ct_eyebrow="Kontakt", ct_h1="Schreiben Sie uns.", ct_lede="Fragen, Ideen, Hilfsangebote, Angebotsanfragen: alles landet im selben Postfach, und ein Mensch antwortet.",
 ct_email="E-Mail", ct_place="Ort", ct_project="Projekt", ct_status="Status", ct_status_v="VoG/ASBL-Eintragung läuft",
 ct_name="Ihr Name", ct_mail="Ihre E-Mail", ct_topic="Ich schreibe wegen", ct_msg="Nachricht",
 ct_o1="Einer Spende", ct_o2="Angebot von Können oder Zeit", ct_o3="Einer Partnerschaft", ct_o4="Eines Softwareprojekts", ct_o5="BacMath.eu", ct_o6="Etwas anderem",
 ct_btn="In meinem E-Mail-Programm öffnen", ct_fine="Dies öffnet Ihr eigenes E-Mail-Programm mit der versandfertigen Nachricht. Nichts wird an diese Website gesendet oder dort gespeichert. Siehe unsere <a href='privacy.html'>Datenschutzerklärung</a>.",
 upd="Zuletzt aktualisiert: 7. November 2026",
 lg_eyebrow="Impressum", lg_h1="Impressum.", lg_lede="Für die Öffentlichkeit, für Partner und für die Behörden.",
 lg_who="Herausgeber",
 lg_k=["Name","Rechtsform","Status","Unternehmensnummer","Standort","Gründer","Mitgründerin","Kontakt","Website","Hosting"],
 lg_v=["Aether EdTech","Vereinigung ohne Gewinnerzielungsabsicht (VoG / ASBL) nach belgischem Recht","<strong>Eintragung läuft</strong> in Brüssel","Ausstehend (ZDU)","Schaerbeek, Brüssel, Belgien","Taqi Abbas","Minha Shahid","<a href='mailto:"+EMAIL+"'>"+EMAIL+"</a>","myaether.org","Server in der Europäischen Union"],
 lg_after="Diese Seite wird um die vollständige Anschrift, die Unternehmensnummer und einen Link zur Satzung ergänzt, sobald die Eintragung abgeschlossen und im <em>Belgischen Staatsblatt</em> veröffentlicht ist.",
 lg_sections=[
  ("Spenden und Bescheinigungen","Bis zum Abschluss der Eintragung kann Aether EdTech keine Steuerbescheinigungen für Spenden ausstellen, und niemand sollte einen Steuerabzug erwarten. Jede Änderung geben wir auf dieser Seite bekannt."),
  ("Inhalte und geistiges Eigentum","Texte, Gestaltung und Code dieser Website gehören Aether EdTech, sofern nicht anders angegeben. Kurze Auszüge dürfen mit Quellenlink zitiert werden. BacMath.eu und andere verlinkte Projekte haben eigene Bedingungen."),
  ("Haftung","Wir achten auf die Richtigkeit dieser Seite, können aber nicht garantieren, dass sie fehlerfrei ist. Für die Inhalte externer Seiten, auf die wir verlinken, sind wir nicht verantwortlich."),
  ("Marken","Google, Microsoft, GoodStack, Adobe, Canva und StartLab Brussels werden hier nur genannt, um ihnen für ihre Unterstützung zu danken. Ihre Namen sind Marken ihrer jeweiligen Inhaber und bedeuten keine Billigung von Aether EdTech."),
  ("Datenschutz","Siehe unsere <a href='privacy.html'>Datenschutzerklärung und DSGVO-Informationen</a> und unsere <a href='cookies.html'>Cookie-Erklärung</a>."),
  ("Anwendbares Recht","Es gilt belgisches Recht. Die Gerichte in Brüssel sind zuständig, unbeschadet zwingender Verbraucherrechte."),
 ],
 pv_eyebrow="Datenschutz &amp; DSGVO", pv_h1="Datenschutzerklärung und DSGVO-Informationen.",
 pv_lede="Kurz gesagt: Wir verfolgen Sie nicht, wir verwenden keine Cookies, und Ihre Daten bleiben in der Europäischen Union.",
 pv_sections=[
  ("Kurzfassung","<ul><li>Keine Cookies, keine Analyse, keine Werbung, keine Tracking-Pixel.</li><li>Wir erheben keine Daten über Ihren Besuch, und nichts wird verkauft oder zu Werbezwecken weitergegeben.</li><li>Diese Website wird auf Servern in der Europäischen Union gehostet.</li><li>Wir übermitteln keine personenbezogenen Daten außerhalb des Europäischen Wirtschaftsraums.</li><li>Sie können uns jederzeit schreiben, um Ihre Rechte auszuüben.</li></ul>"),
  ("Wer verantwortlich ist","Verantwortlicher ist <strong>Aether EdTech</strong>, Schaerbeek, Brüssel, Belgien (Eintragung läuft). Kontakt: <a href='mailto:"+EMAIL+"'>"+EMAIL+"</a>. Wir sind nicht verpflichtet, einen Datenschutzbeauftragten zu benennen; Datenschutzfragen gehen an dieselbe Adresse."),
  ("Was wir verarbeiten, warum und wie lange","<h3>Besuch dieser Website</h3><p>Wir erheben keine personenbezogenen Daten von Besuchern. Wie jeder Webserver kann unser Hosting-Anbieter technisch Ihre IP-Adresse und den Zeitpunkt Ihrer Anfrage in kurzlebigen Sicherheitsprotokollen festhalten, die in der EU liegen, damit die Seite sicher laufen kann. Wir verwenden diese Protokolle nicht, um Besucher zu identifizieren oder Profile zu bilden.</p><h3>Sprachauswahl</h3><p>Die Sprache der Seite wird von einem kleinen Skript gewählt, das die Spracheinstellung Ihres Browsers auf Ihrem eigenen Gerät liest. Diese Information wird nicht an uns gesendet, und nichts wird gespeichert.</p><h3>Wenn Sie uns schreiben</h3><p>Wir verarbeiten Ihren Namen, Ihre E-Mail-Adresse und den Inhalt Ihrer Nachricht, um Ihnen zu antworten. Rechtsgrundlage: unser berechtigtes Interesse, Anfragen zu beantworten, oder vorvertragliche Maßnahmen auf Ihre Anfrage (Art. 6 Abs. 1 lit. f und b DSGVO). Wir bewahren Nachrichten nur so lange auf, wie zur Bearbeitung nötig, normalerweise nicht länger als 24 Monate nach dem letzten Austausch.</p><h3>Spenden per Überweisung</h3><p>Ihre Bank übermittelt uns Ihren Namen, Ihre Kontonummer und den Betrag. Wir bewahren dies zur Erfüllung unserer buchhalterischen Pflichten nach belgischem Recht auf (Art. 6 Abs. 1 lit. c DSGVO), normalerweise sieben Jahre.</p>"),
  ("Wo Ihre Daten liegen","Unsere Website und unser Postfach werden innerhalb der Europäischen Union betrieben. Wir wählen Anbieter, die Daten in der EU/im EWR verarbeiten und nur auf unsere Weisung handeln. Wir übermitteln keine personenbezogenen Daten in Länder außerhalb des Europäischen Wirtschaftsraums. Sollte sich das je ändern, informieren wir Sie zuvor und stützen uns nur auf die nach der DSGVO erforderlichen Garantien."),
  ("Ihre Rechte nach der DSGVO","<p>Sie haben das Recht auf:</p><ul><li>Auskunft über die zu Ihnen gespeicherten personenbezogenen Daten;</li><li>Berichtigung unrichtiger Daten;</li><li>Löschung Ihrer Daten (&bdquo;Recht auf Vergessenwerden&ldquo;);</li><li>Einschränkung der Verarbeitung und Widerspruch;</li><li>Erhalt Ihrer Daten in einem übertragbaren Format;</li><li>Widerruf Ihrer Einwilligung jederzeit, soweit die Verarbeitung darauf beruht.</li></ul><p>Schreiben Sie an <a href='mailto:"+EMAIL+"'>"+EMAIL+"</a>. Wir antworten innerhalb eines Monats. Wir können Sie bitten, Ihre Identität zu bestätigen.</p>"),
  ("Beschwerderecht","Wenn Sie glauben, dass wir Ihre Daten rechtswidrig verarbeiten, können Sie sich bei der belgischen Datenschutzbehörde beschweren: Rue de la Presse 35, 1000 Brüssel, <a href='mailto:contact@apd-gba.be'>contact@apd-gba.be</a>, <a href='https://www.autoriteprotectiondonnees.be' rel='noopener'>autoriteprotectiondonnees.be</a>. Wir würden uns freuen, wenn wir vorher die Gelegenheit bekämen, das Problem zu lösen."),
  ("Kinder","Wir erheben über diese Website nicht wissentlich personenbezogene Daten von Kindern. BacMath.eu ist eine eigene Website mit eigenen Datenschutzhinweisen."),
  ("Links zu anderen Websites","Links zu BacMath.eu und anderen Seiten führen zu Diensten, die wir für diese Website nicht kontrollieren. Dort gelten deren eigene Datenschutzerklärungen."),
  ("Sicherheit","Wir beschränken den Zugriff auf das Postfach auf die Gründer und nutzen seriöse Anbieter in der EU. Keine Methode ist vollkommen sicher; wir informieren Sie und die Behörde, wenn eine Datenpanne Sie betrifft, wie es die DSGVO verlangt."),
  ("Änderungen","Wir aktualisieren diese Seite, wenn sich unsere Praxis ändert, und nennen oben das Datum. Wesentliche Änderungen geben wir auf der Startseite bekannt."),
 ],
 ck_eyebrow="Cookies", ck_h1="Cookie-Erklärung.", ck_lede="Diese Website verwendet keine Cookies.",
 ck_sections=[
  ("Was wir verwenden","Nichts. Wir setzen keine Cookies und nutzen weder lokalen Speicher noch Fingerprinting, Analyse, Werbung oder Social-Media-Widgets."),
  ("Schriften und Ressourcen","Schriften, Bilder und Skripte werden von dieser Website selbst ausgeliefert. Wir laden sie nicht von Dritten, sodass Ihr Browser bei einem Besuch keine anderen Unternehmen kontaktiert."),
  ("Falls sich das ändert","Sollten wir je etwas hinzufügen, das Ihre Einwilligung erfordert, fragen wir vor dem Laden nach, wie es belgisches und EU-Recht verlangen, und aktualisieren diese Seite."),
  ("Weitere Informationen","Siehe die <a href='privacy.html'>Datenschutzerklärung und DSGVO-Informationen</a>."),
 ],
)

from overrides import OV
for _L in LANGS:
    S[_L].update(OV[_L])
for _L in LANGS:
    assert set(S[_L]) == set(S["en"]), (_L, set(S[_L]) ^ set(S["en"]))

# =====================================================================
# TEMPLATES
# =====================================================================
def supporters_html():
    return "<div class='supporters' role='list'>" + "".join(f"<span role='listitem'>{n}</span>" for n in SUPPORTERS) + "</div>"

def cy_cards(items):
    return "".join(f"<div class='cy'><span class='cy-n'>{i+1:02d}</span><h3>{h}</h3><p>{p}</p></div>" for i,(h,p) in enumerate(items))

def sections_prose(secs, first_plain=False):
    out = ""
    for i, (h, body) in enumerate(secs):
        body = body if body.lstrip().startswith("<") else f"<p>{body}</p>"
        style = ' style="margin-top:0"' if i == 0 else ''
        out += f"<h2{style}>{h}</h2>{body}"
    return out

def P(name, L):
    t = lambda k: S[L][k]
    if name == "index":
        return f'''
<section class="hero"><div class="wrap hero-grid">
  <div>
    <span class="eyebrow">{t("h_eyebrow")}</span>
    <h1>{t("h_h1")}</h1>
    <p class="lede">{t("h_lede")}</p>
    <div class="btn-row"><a class="btn" href="services.html">{t("h_b2")}</a><a class="btn ghost" href="projects.html">{t("h_b1")}</a></div>
    <p class="fineprint hero-reg">{t("h_pill")}</p>
  </div>
  <figure class="emblem">
    <img src="../assets/img/logo-lg.png" alt="Aether EdTech" width="260" height="260" decoding="async">
  </figure>
</div></section>

<section class="sec tint"><div class="wrap split">
  <div class="sticky"><span class="eyebrow">{t("w_eyebrow")}</span><h2>{t("w_h2")}</h2><p class="lede">{t("w_lede")}</p></div>
  <ol class="rows">
    <li><div><h3>{t("r1h")}</h3><p>{t("r1p")}</p></div></li>
    <li><div><h3>{t("r2h")}</h3><p>{t("r2p")}</p></div></li>
    <li><div><h3>{t("r3h")}</h3><p>{t("r3p")}</p></div></li>
  </ol>
</div></section>

<section class="sec cyber"><div class="wrap">
  <div class="cyber-head"><span class="eyebrow">{t("cy_eyebrow")}</span><h2>{t("cy_h2")}</h2><p class="lede">{t("cy_lede")}</p></div>
  <div class="cyber-grid">{cy_cards(t("cy_items"))}</div>
  <p class="btn-row"><a class="btn" href="contact.html">{t("cy_btn")}</a><a class="btn ghost" href="services.html">{t("h_b2")}</a></p>
</div></section>

<section class="sec feature"><div class="wrap feature-grid">
  <div>
    <span class="eyebrow">{t("fe_eyebrow")}</span>
    <h2>BacMath.eu</h2>
    <p>{t("fe_p1")}</p><p>{t("fe_p2")}</p>
    <ul class="ticks"><li>{t("fe_t1")}</li><li>{t("fe_t2")}</li><li>{t("fe_t3")}</li></ul>
    <div class="btn-row"><a class="btn light" href="https://bacmath.eu" rel="noopener">{t("fe_b1")}</a><a class="btn outline-light" href="projects.html">{t("fe_b2")}</a></div>
  </div>
  <p class="formula">{t("fe_q")}<small>{t("fe_cap")}</small></p>
</div></section>

<section class="sec"><div class="wrap">
  <div class="sec-head"><span class="eyebrow">{t("th_eyebrow")}</span><h2>{t("th_h2")}</h2><p class="lede">{t("th_lede")}</p></div>
  {supporters_html()}
  <p class="fineprint">{t("th_fine")}</p>
</div></section>

<section class="sec band"><div class="wrap band-grid">
  <div><h2>{t("b_h2")}</h2><p>{t("b_p")}</p></div>
  <div class="btn-row"><a class="btn light" href="support.html">{t("b_btn")}</a></div>
</div></section>
'''
    if name == "about":
        return f'''
<section class="page-head"><div class="wrap"><span class="eyebrow">{t("ab_eyebrow")}</span><h1>{t("ab_h1")}</h1><p class="lede">{t("ab_lede")}</p></div></section>
<section class="sec mv"><div class="wrap">
  <div class="sec-head"><span class="eyebrow">{t("mv_eyebrow")}</span><h2>{t("mv_h")}</h2></div>
  <div class="mv-grid">
    <div class="mv-card"><h3>{t("mis_h")}</h3><p>{t("mis_p")}</p></div>
    <div class="mv-card"><h3>{t("vis_h")}</h3><p>{t("vis_p")}</p></div>
  </div>
</div></section>
<section class="sec tint"><div class="wrap split">
  <div class="sticky"><span class="eyebrow">{t("c_eyebrow")}</span><h2>{t("c_h2")}</h2></div>
  <div class="prose"><p>{t("c_p1")}</p><p>{t("c_p2")}</p>
    <h3>{t("bel_h")}</h3>
    <ul><li>{t("bel1")}</li><li>{t("bel2")}</li><li>{t("bel3")}</li><li>{t("bel4")}</li></ul></div>
</div></section>
<section class="sec"><div class="wrap">
  <div class="sec-head"><span class="eyebrow">{t("fo_eyebrow")}</span><h2>{t("fo_h2")}</h2></div>
  <div class="people">
    <article class="person"><div class="monogram" aria-hidden="true">TA</div><h3>Taqi Abbas</h3><p class="role">{t("p1_role")}</p><p class="prof">{t("p1_prof")}</p><p>{t("p1_bio")}</p></article>
    <article class="person"><div class="monogram" aria-hidden="true">MS</div><h3>Minha Shahid</h3><p class="role">{t("p2_role")}</p><p class="prof">{t("p2_prof")}</p><p>{t("p2_bio")}</p></article>
  </div>
</div></section>
<section class="sec"><div class="wrap two">
  <div class="col-block"><h3>{t("wh_h")}</h3><p>{t("wh_p")}</p></div>
  <div class="col-block" id="status"><h3>{t("st_h")}</h3><div class="note"><p>{t("st_p")}</p></div></div>
</div></section>
'''
    if name == "projects":
        return f'''
<section class="page-head"><div class="wrap"><span class="eyebrow">{t("pr_eyebrow")}</span><h1>{t("pr_h1")}</h1><p class="lede">{t("pr_lede")}</p></div></section>
<section class="sec feature"><div class="wrap feature-grid">
  <div><span class="eyebrow">{t("pr_live")}</span><h2>BacMath.eu</h2><p>{t("pr_p1")}</p><p>{t("pr_p2")}</p>
    <div class="btn-row"><a class="btn light" href="https://bacmath.eu" rel="noopener">{t("pr_b1")}</a><a class="btn outline-light" href="support.html">{t("pr_b2")}</a></div></div>
  <p class="formula">{t("pr_q")}<small>{t("pr_cap")}</small></p>
</div></section>
<section class="sec tint"><div class="wrap split">
  <div class="sticky"><span class="eyebrow">{t("pr_next_eyebrow")}</span><h2>{t("pr_next_h")}</h2></div>
  <div class="prose"><p class="lede" style="margin-top:0">{t("pr_next_p")}</p><p><a class="btn ghost" href="contact.html">{t("ot_btn")}</a></p></div>
</div></section>
<section class="sec"><div class="wrap split">
  <div class="sticky"><span class="eyebrow">{t("ot_eyebrow")}</span><h2>{t("ot_h2")}</h2></div>
  <div class="prose"><p>{t("ot_p1")}</p><p>{t("ot_p2")}</p><h3>{t("ot_h3")}</h3><p>{t("ot_p3")}</p><p><a class="btn" href="contact.html">{t("ot_btn")}</a></p></div>
</div></section>
'''
    if name == "services":
        return f'''
<section class="page-head"><div class="wrap"><span class="eyebrow">{t("sv_eyebrow")}</span><h1>{t("sv_h1")}</h1><p class="lede">{t("sv_lede")}</p></div></section>
<section class="sec"><div class="wrap split">
  <div class="sticky"><span class="eyebrow">{t("sv_how")}</span><h2>{t("sv_h2")}</h2></div>
  <div>
    
    <ol class="rows">
      <li><div><h3>{t("sv1h")}</h3><p>{t("sv1p")}</p></div></li>
      <li><div><h3>{t("sv2h")}</h3><p>{t("sv2p")}</p></div></li>
      <li><div><h3>{t("sv3h")}</h3><p>{t("sv3p")}</p></div></li>
      <li><div><h3>{t("sv4h")}</h3><p>{t("sv4p")}</p></div></li>
    </ol>
  </div>
</div></section>
<section class="sec cyber" id="cybersecurity"><div class="wrap">
  <div class="cyber-head"><span class="eyebrow">{t("cs_eyebrow")}</span><h2>{t("cs_h2")}</h2><p class="lede">{t("cs_lede")}</p></div>
  <div class="cyber-grid cols3">{cy_cards(t("cs_items"))}</div>
  <div class="note" style="margin-top:2.5rem"><p>{t("cs_note")}</p></div>
  <p class="btn-row"><a class="btn" href="contact.html">{t("cs_btn")}</a></p>
</div></section>
<section class="sec tint"><div class="wrap">
  <div class="sec-head"><span class="eyebrow">{t("pc_eyebrow")}</span><h2>{t("pc_h2")}</h2></div>
  <div class="cyber-grid process">{cy_cards(t("pc_items"))}</div>
</div></section>
<section class="sec band"><div class="wrap band-grid"><div><h2>{t("sv_b_h")}</h2><p>{t("sv_b_p")}</p></div><div class="btn-row"><a class="btn light" href="contact.html">{t("sv_b_btn")}</a></div></div></section>
'''
    if name == "support":
        subj = t("don_subj").replace(" ", "%20")
        return f'''
<section class="page-head"><div class="wrap"><span class="eyebrow">{t("su_eyebrow")}</span><h1>{t("su_h1")}</h1><p class="lede">{t("su_lede")}</p></div></section>
<section class="sec"><div class="wrap"><div class="ways">
  <article class="way"><h3><span>01</span> {t("w1h")}</h3><p>{t("w1p")}</p><p><a href="mailto:{EMAIL}?subject={subj}">{t("w1a")}</a></p></article>
  <article class="way"><h3><span>02</span> {t("w2h")}</h3><p>{t("w2p")}</p><p><a href="contact.html">{t("w2a")}</a></p></article>
  <article class="way"><h3><span>03</span> {t("w3h")}</h3><p>{t("w3p")}</p><p><a href="contact.html">{t("w3a")}</a></p></article>
  <article class="way"><h3><span>04</span> {t("w4h")}</h3><p>{t("w4p")}</p><p><a href="projects.html">{t("w4a")}</a></p></article>
</div></div></section>
<section class="sec tint"><div class="wrap split">
  <div class="sticky"><span class="eyebrow">{t("sg_eyebrow")}</span><h2>{t("sg_h2")}</h2></div>
  <div class="prose"><p>{t("sg_p")}</p><ul><li>{t("sg1")}</li><li>{t("sg2")}</li><li>{t("sg3")}</li></ul></div>
</div></section>
<section class="sec"><div class="wrap">
  <div class="sec-head"><span class="eyebrow">{t("so_eyebrow")}</span><h2>{t("so_h2")}</h2><p class="lede">{t("so_note")}</p></div>
  {supporters_html()}
  <p class="fineprint">{t("th_fine")}</p>
</div></section>
'''
    if name == "contact":
        return f'''
<section class="page-head"><div class="wrap"><span class="eyebrow">{t("ct_eyebrow")}</span><h1>{t("ct_h1")}</h1><p class="lede">{t("ct_lede")}</p></div></section>
<section class="sec"><div class="wrap split">
  <div><dl class="facts">
    <dt>{t("ct_email")}</dt><dd><a href="mailto:{EMAIL}">{EMAIL}</a></dd>
    <dt>{t("ct_place")}</dt><dd>{t("id1v")}</dd>
    <dt>{t("ct_ent")}</dt><dd>{t("ct_ent_v")}</dd>
  </dl></div>
  <div>
  <form class="contact" id="contact-form" action="mailto:{EMAIL}" method="post" enctype="text/plain" novalidate data-lang="{L}" data-sending="{t("ct_sending")}" data-btn="{t("ct_btn")}">
    <div><label for="name">{t("ct_name")}</label><input id="name" name="name" autocomplete="name" maxlength="100" required></div>
    <div><label for="email">{t("ct_mail")}</label><input id="email" name="email" type="email" autocomplete="email" maxlength="200" required></div>
    <div><label for="topic">{t("ct_topic")}</label><select id="topic" name="topic"><option>{t("ct_o4")}</option><option>{t("ct_o3")}</option><option>{t("ct_o5")}</option><option>{t("ct_o1")}</option><option>{t("ct_o2")}</option><option>{t("ct_o6")}</option></select></div>
    <div><label for="msg">{t("ct_msg")}</label><textarea id="msg" name="message" maxlength="5000" minlength="10" required></textarea></div>
    <div class="hp" aria-hidden="true"><label for="website">Website</label><input id="website" name="website" tabindex="-1" autocomplete="off"></div>
    <div><button class="btn" type="submit">{t("ct_btn")}</button><p class="fineprint">{t("ct_fine")}</p></div>
    <div class="form-msg" role="status" aria-live="polite" hidden></div>
    <template id="msg-ok">{t("ct_ok")}</template>
    <template id="msg-err">{t("ct_err")}</template>
  </form>
  </div>
</div></section>
'''
    if name == "legal":
        facts = "".join(f"<dt>{k}</dt><dd>{v}</dd>" for k, v in zip(t("lg_k"), t("lg_v")))
        return f'''
<section class="page-head"><div class="wrap"><span class="eyebrow">{t("lg_eyebrow")}</span><h1>{t("lg_h1")}</h1><p class="lede">{t("lg_lede")}</p></div></section>
<section class="sec"><div class="wrap prose">
  <h2 style="margin-top:0">{t("lg_who")}</h2>
  <dl class="facts" style="grid-template-columns:12rem 1fr">{facts}</dl>
  <p style="margin-top:1.5rem">{t("lg_after")}</p>
  {sections_prose(t("lg_sections"))}
  <p class="fineprint">{t("upd")}</p>
</div></section>
'''
    if name == "privacy":
        return f'''
<section class="page-head"><div class="wrap"><span class="eyebrow">{t("pv_eyebrow")}</span><h1>{t("pv_h1")}</h1><p class="lede">{t("pv_lede")}</p></div></section>
<section class="sec"><div class="wrap prose">
  <p class="fineprint" style="margin-top:0">{t("upd")}</p>
  {sections_prose(t("pv_sections"))}
</div></section>
'''
    if name == "cookies":
        return f'''
<section class="page-head"><div class="wrap"><span class="eyebrow">{t("ck_eyebrow")}</span><h1>{t("ck_h1")}</h1><p class="lede">{t("ck_lede")}</p></div></section>
<section class="sec"><div class="wrap prose">
  <p class="fineprint" style="margin-top:0">{t("upd")}</p>
  {sections_prose(t("ck_sections"))}
</div></section>
'''

def chrome_head(L, name):
    t = lambda k: S[L][k]
    nav = ""
    for p in ["index", "about", "projects", "services", "contact"]:
        cur = ' aria-current="page"' if p == name else ""
        nav += f'<li><a href="{p}.html"{cur}>{t("n_"+p)}</a></li>\n'
    cur = ' aria-current="page"' if name == "support" else ""
    nav += f'<li><a class="give" href="support.html"{cur}>{t("n_support")}</a></li>'
    cur_attr = ' aria-current="true"'
    lm = ''.join('<li><a href="../%s/%s.html" hreflang="%s" lang="%s"%s>%s</a></li>' % (l, name, l, l, cur_attr if l == L else '', LANG_NAME[l]) for l in LANGS)
    nav += '<li class="lang-m"><span class="lang-m-l">' + t('langlabel') + '</span><ul class="lang" aria-label="' + t('langlabel') + '">' + lm + '</ul></li>'
    langs = ""
    for l in LANGS:
        cur = ' aria-current="true"' if l == L else ""
        langs += f'<li><a href="../{l}/{name}.html" hreflang="{l}" lang="{l}" title="{LANG_NAME[l]}"{cur}>{l.upper()}</a></li>'
    alts = "".join(f'<link rel="alternate" hreflang="{l}" href="https://myaether.org/{l}/{name}.html">\n' for l in LANGS)
    alts += f'<link rel="alternate" hreflang="x-default" href="https://myaether.org/">\n'
    title, desc = t("t_"+name), t("d_"+name)
    return f'''<!DOCTYPE html>
<html lang="{L}">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>{title}</title>
<meta name="description" content="{desc}">
<meta name="theme-color" content="#f7f2e8">
<meta name="referrer" content="no-referrer">
<link rel="canonical" href="https://myaether.org/{L}/{name}.html">
{alts}<meta property="og:title" content="{title}">
<meta property="og:description" content="{desc}">
<meta property="og:type" content="website">
<meta property="og:locale" content="{ {'en':'en_BE','fr':'fr_BE','nl':'nl_BE','de':'de_BE'}[L] }">
<link rel="icon" href="../assets/favicon.png" type="image/png">
<link rel="stylesheet" href="../css/style.css">
</head>
<body>
<a class="skip" href="#main">{t("skip")}</a>
<header class="site-head">
  <div class="wrap head-in">
    <a class="brand" href="index.html" aria-label="Aether EdTech">
      <img src="../assets/img/logo.png" alt="" width="44" height="44">
      <span><b>Aether EdTech</b><small>ASBL · Brussels</small></span>
    </a>
    <nav class="nav" id="nav" aria-label="Main"><ul>
{nav}
    </ul></nav>
    <ul class="lang" aria-label="{t("langlabel")}">{langs}</ul>
    <button class="menu-btn" aria-expanded="false" aria-controls="nav">{t("menu")}</button>
  </div>
</header>
<main id="main">
'''

def chrome_foot(L):
    t = lambda k: S[L][k]
    return f'''</main>
<footer class="site-foot">
  <div class="wrap">
    <div class="foot-grid">
      <div><div class="foot-brand"><img src="../assets/img/logo.png" alt="" width="44" height="44">Aether EdTech</div><p>{t("f_tag")}</p></div>
      <div><h4>{t("f_explore")}</h4><ul>
        <li><a href="about.html">{t("n_about")}</a></li><li><a href="projects.html">{t("n_projects")}</a></li>
        <li><a href="services.html">{t("n_services")}</a></li><li><a href="support.html">{t("n_support")}</a></li></ul></div>
      <div><h4>{t("n_projects")}</h4><ul>
        <li><a href="projects.html">{t("n_projects")}</a></li><li><a href="https://bacmath.eu" rel="noopener">BacMath.eu</a></li><li><a href="services.html#cybersecurity">{t("cs_eyebrow")}</a></li><li><a href="contact.html">{t("f_partner")}</a></li></ul></div>
      <div><h4>{t("f_legal")}</h4><ul>
        <li><a href="legal.html">{t("t_legal").split(" · ")[0]}</a></li>
        <li><a href="privacy.html">{t("t_privacy").split(" · ")[0]}</a></li>
        <li><a href="cookies.html">{t("t_cookies").split(" · ")[0]}</a></li></ul></div>
      <div><h4>{t("f_reach")}</h4><ul><li><a href="mailto:{EMAIL}">{EMAIL}</a></li></ul></div>
    </div>
    <div class="legal-line"><p>{t("f_status")}</p><p>{t("f_eu")}</p></div>
  </div>
</footer>
<script src="../js/main.js"></script>
</body>
</html>
'''

ROUTER = '''<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Aether EdTech</title>
<meta name="description" content="Aether EdTech, Brussels. English · Français · Nederlands · Deutsch">
<meta name="referrer" content="no-referrer">
<link rel="icon" href="assets/favicon.png" type="image/png">
<link rel="stylesheet" href="css/style.css">
__ALTS__
<script>
(function () {
  var ok = ['en', 'fr', 'nl', 'de'], list = [];
  try { list = (navigator.languages && navigator.languages.length) ? navigator.languages : [navigator.language || navigator.userLanguage]; } catch (e) {}
  var pick = 'en';
  for (var i = 0; i < list.length; i++) {
    var c = String(list[i] || '').toLowerCase().split('-')[0];
    if (ok.indexOf(c) !== -1) { pick = c; break; }
  }
  location.replace(pick + '/');
})();
</script>
<noscript><style>.router-wait{display:none}</style></noscript>
</head>
<body class="router">
<main class="router-box">
  <img src="assets/img/logo.png" alt="" width="96" height="96">
  <h1>Aether EdTech</h1>
  <p class="router-wait">…</p>
  <ul class="router-langs">
    <li><a href="en/" hreflang="en" lang="en">English</a></li>
    <li><a href="fr/" hreflang="fr" lang="fr">Français</a></li>
    <li><a href="nl/" hreflang="nl" lang="nl">Nederlands</a></li>
    <li><a href="de/" hreflang="de" lang="de">Deutsch</a></li>
  </ul>
</main>
</body>
</html>
'''

def main():
    for L in LANGS:
        d = os.path.join(OUT, L)
        os.makedirs(d, exist_ok=True)
        for name in PAGES:
            with open(os.path.join(d, name + ".html"), "w", encoding="utf-8") as f:
                f.write(chrome_head(L, name) + P(name, L) + chrome_foot(L))
    alts = "".join(f'<link rel="alternate" hreflang="{l}" href="https://myaether.org/{l}/">' for l in LANGS)
    with open(os.path.join(OUT, "index.html"), "w", encoding="utf-8") as f:
        f.write(ROUTER.replace("__ALTS__", alts))
    # remove stale top-level pages from the previous single-language build
    for name in PAGES[1:]:
        p = os.path.join(OUT, name + ".html")
        if os.path.exists(p): os.remove(p)
    # sanity: every key used exists in every language
    base = set(S["en"])
    for L in LANGS:
        miss = base ^ set(S[L])
        assert not miss, (L, miss)
    print("built", len(LANGS) * len(PAGES), "pages")

main()
